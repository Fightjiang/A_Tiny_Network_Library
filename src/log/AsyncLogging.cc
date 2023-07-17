#include "./log/AsyncLogging.h"
#include "./base/Timestamp.h" 
#include <stdio.h>

AsyncLogging::AsyncLogging(int flushInterval)
    : flushInterval_(flushInterval), // 刷新到磁盘里的时间间隔
      running_(true),               // 是否在运行中 
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"), // 只是指定线程运行的函数和线程名，还并没有启动
      mutex_(),
      cond_(),
      currentBuffer_(new Buffer),   // 当前存储日志信息的 Large Buffer
      nextBuffer_(new Buffer),      // 备用的 Large Buffer 
      bufferVec_()                  // 前端存储的日志队列，默认为 16 个
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    bufferVec_.reserve(16); 
    thread_.start();
}

// 前端会有多个线程同时 append 数据进来，故需要加锁
void AsyncLogging::append(const char* logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer_->avail() > len) // 缓冲区剩余空间足够则直接写入
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        // 当前缓冲区空间不够，启用备用缓冲区
        bufferVec_.push_back(std::move(currentBuffer_));
        if (nextBuffer_) 
        {
            currentBuffer_ = std::move(nextBuffer_);
        } 
        else 
        {
            // 备用缓冲区已经使用过了，重新分配新的缓冲区，这种情况很少
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        // 唤醒后台线程将 日志信息 写入磁盘
        cond_.notify_one();
    }
}

// 后台线程运行的函数
void AsyncLogging::threadFunc()
{
    // output有写入磁盘的接口
    LogFile output ;
    // 后端缓冲区，用于与前端的缓冲区进行交互，currentBuffer nextBuffer
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    // 缓冲区队列置为16个，用于和前端缓冲区队列进行交换，减少锁的占用时间
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16); // reserve 不创建对象，避免多次重复扩容
    while (running_)
    {
        {
            // 互斥锁保护，这样别的线程在这段时间就无法向前端Buffer数组写入数据
            std::unique_lock<std::mutex> lock(mutex_);
            while (running_ && bufferVec_.empty() && currentBuffer_->length() == 0)
            {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_)); // 默认等待三秒也会自动解除阻塞
            }

            // 此时正使用的 currentbuffer 也放入 currentbuffer 数组中（没写完也放进去，避免等待太久才刷新一次）
            bufferVec_.push_back(std::move(currentBuffer_));
            // 归还正使用缓冲区
            currentBuffer_ = std::move(newBuffer1);
            // 后端缓冲区队列 和 前端缓冲区队列交换
            buffersToWrite.swap(bufferVec_);
            // 检查备用缓冲区是否启用，启用则进行交换
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }
        //锁已经被释放，后台线程遍历所有 buffer，将其写入文件
        for (const auto& buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }

        // 只保留两个缓冲区
        if (buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }

        // 归还newBuffer1缓冲区
        if (!newBuffer1)
        {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        // 归还newBuffer2缓冲区
        if (!newBuffer2)
        {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear(); // 清空后端缓冲区队列
        output.flush(); //清空文件缓冲区
    }
    output.flush();
}