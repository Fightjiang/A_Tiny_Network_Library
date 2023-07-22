#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "../log/Logging.h"

// 前置声明，可以不引用头文件暴露文件信息
class EventLoop ; 
class TimeStamp ; 

// Channel 类理解为是 fd 的保姆类
class Channel : noncopyable 
{
public : 
    using EventCallback = std::function<void()> ;
    using ReadEventCallback = std::function<void(Timestamp)> ;

    Channel(EventLoop *loop, int fd) ;
    ~Channel() ;

    // fd得到 poller 通知以后，处理事件的回调函数
    void handleEvent(Timestamp receiveTime);

    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    int fd() const { return fd_; }                    // 返回封装的fd
    int events() const { return events_; }            // 返回在监听的具体感兴趣的事件
    void set_revents(int revt) { revents_ = revt; }   // 主要是提供给 Poller ，设置返回的发生事件

    // 设置fd相应的事件状态，update() 其本质调用 epoll_ctl 设置对应 epoll 需要监听的 fd_ 上发生的事件
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ &= kNoneEvent; update(); }

     // 返回fd当前被 Epoller 监听的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    /**
     * for Epoller
     * const int kNew = -1;     // fd 还未被 Epoller 监视 
     * const int kAdded = 1;    // fd 正被 Epoller 监视中
     * const int kDeleted = 2;  // fd 被移除 Epoller
    */ 
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop one thread
    EventLoop* ownerLoop() { return loop_; }
    void remove();
    
    // 防止当 channel 执行回调函数时被被手动 remove 掉
    void tie(const std::shared_ptr<void>&);

private : 
    
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;
 
    EventLoop *loop_;   // 当前 Channel 属于的EventLoop
    const int fd_;      // fd, Poller 监听对象，此 Channel 可以理解为是该 fd_ 的保姆
    int events_;        // 注册fd感兴趣的事件
    int revents_;       // Poller 返回的具体发生的事件, 获知 fd 最终发生的具体的事件 revents
    int index_;         // 在 Poller 上注册的情况

    // 非常巧妙的操作，弱指针指向TcpConnection(必要时升级为shared_ptr多一份引用计数，避免用户误删)
    // tied_ 标记此 Channel 是否被调用过 Channel::tie 方法
    std::weak_ptr<void> tie_;    
    bool tied_;  

    // 保存事件到来时需要执行的回调函数，这里的回调函数是被用户定义的然后通过 TcpConnection 一层一层定义传递过来的
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

#endif