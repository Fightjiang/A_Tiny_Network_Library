#ifndef ASYNC_LOGGING_H
#define ASYNC_LOGGING_H

#include "../base/noncopyable.h"
#include "../base/Thread.h" 
#include "../base/FixedBuffer.h"
#include "LogFile.h"
#include <vector>
#include <mutex>
#include <condition_variable>

// 后台日志模块，单例模式
class AsyncLogging  
{
public : 
    static AsyncLogging& Instance(){
        static AsyncLogging instance ; 
        return instance ; 
    }

    // 前端调用 append 将日志写入到 buffer_ 队列中
    void append(const char* logline, int len);

    // void start()
    // {
    //     if(running_ == false) {
    //         running_ = true;
    //         thread_.start();
    //     }
    // }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private :

    AsyncLogging(int flushInterval = 3);

    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    void threadFunc() ;

    const int flushInterval_;  
    std::atomic<bool> running_; 

    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector bufferVec_;

};

#endif