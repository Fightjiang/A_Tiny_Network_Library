#include "./net/EventLoop.h"
#include "./net/EventLoopThread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name) // 新线程绑定执行此函数
    , mutex_()
    , cond_()
    , callback_(cb) // 传入的线程初始化回调函数，用户自定义的
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    // 调用startLoop即开启一个新线程执行 threadFunc 函数
    thread_.start();
    EventLoop *loop = nullptr;
    {
        // 等待新线程执行 threadFunc 完毕，所以使用 cond_.wait
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock , [&]() { return loop_ != nullptr ; }) ; 
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop ;
    if (callback_)// 用户自定义的回调函数
    {
        callback_(&loop) ;
    }

    {
        std::unique_lock<std::mutex> lock(mutex_) ;
        this->loop_ = &loop ; // 等到生成EventLoop对象之后才唤醒
        cond_.notify_one() ;
    }
    // 执行EventLoop的loop() 开启了底层的 Epoller 的 epoll_wait() 
    loop.loop();   
    // loop是一个事件循环，如果往下执行说明停止了事件循环，需要关闭eventLoop
    // 此处是获取互斥锁再置loop_为空
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}