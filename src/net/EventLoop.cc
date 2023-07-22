#include "./net/EventLoop.h"
#include "./net/Epoller.h"
#include "./log/Logging.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include <fcntl.h>

// 防止一个线程创建多个EventLoop (thread_local)
__thread EventLoop *t_loopInThisThread = nullptr ;

// 定义默认的Epoller IO复用接口的超时时间
const int kPollTimeMs = 10000 ;

// eventFd 作为唤醒的 wakeup Fd , 在 wakeupFd 写入则会唤醒对应的 loop 阻塞住的 epoll_wait  
int createEventfd()
{
    int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evfd < 0)
    {
        LOG_FATAL("eventfd error: %d",errno);
    }
    return evfd;
}

EventLoop::EventLoop() : 
    looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    epoller_(new Epoller(this)),
    // timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(nullptr)
{
    if (t_loopInThisThread)
    {
        LOG_FATAL("The thread %d already exists EventLoop " , threadId_) ;
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件的回调函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupChannel的EPOLLIN事件 , 因为是唤醒 loop 
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    // channel移除所有感兴趣事件
    wakeupChannel_->disableAll();
    // 将channel从EventLoop中删除
    wakeupChannel_->remove();
    // 关闭 wakeupFd_
    ::close(wakeupFd_);
    // 指向EventLoop指针为空
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop start looping") ;

    while (!quit_)
    {
        // 清空activeChannels_
        activeChannels_.clear(); 
        pollReturnTime_ = epoller_->epollWait(kPollTimeMs, &activeChannels_);
        // 执行当前EventLoop事件循环需要处理的回调操作
        for (Channel *channel : activeChannels_)
        {
            channel->handleEvent(pollReturnTime_);
        }
        // 也可能存在其他线程 wakeup 该线程的 epoll ，然后执行对应的回调函数
        // 比如主线程，分发给 subloop 执行对应的回调函数，在 std::vector<Functor> pendingFunctors_ 之中
        doPendingFunctors();
    }
    looping_ = false;    
}

void EventLoop::quit()
{
    quit_ = true;

    /**
     * TODO:生产者消费者队列派发方式和muduo的派发方式
     * 有可能是别的线程调用quit(调用线程不是生成EventLoop对象的那个线程)
     * 比如在工作线程(subLoop)中调用了IO线程(mainLoop)
     * 这种情况会需要唤醒主线程
     */
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在 eventLoop 中执行回调函数
void EventLoop::runInLoop(Functor cb)
{
    // 是否在当前线程中
    if (isInLoopThread())
    {
        cb();
    }
    // 在非当前eventLoop线程中执行回调函数，需要唤醒evevntLoop所在线程
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); // 使用了std::move
    }

    // 唤醒相应的，需要执行上面回调操作的loop线程
    /**  
     * std::atomic_bool callingPendingFunctors_ 标志当前loop是否正在执行回调操作，
     * 因为在执行回调的过程可能会加入新的回调，则这个时候也需要唤醒，否则就会发生有事件到来但是仍被阻塞住的情况
     */
    if (!isInLoopThread() || callingPendingFunctors_)
    { 
        wakeup();
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup writes %d bytes instead of 8" , n);
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8" , n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    epoller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    epoller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return epoller_->hasChannel(channel);    
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    // 通过这种交换的方式，减少互斥锁的加锁时间
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor();
    }

    callingPendingFunctors_ = false;
}