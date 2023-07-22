#include "./net/Channel.h"
#include "./net/EventLoop.h"

// Channel 和 Epoller 之间的联系，都是通过 EventLoop 来完成的
Channel::Channel(EventLoop *loop, int fd)
    :   loop_(loop),
        fd_(fd),
        events_(0),
        revents_(0),
        index_(-1),
        tied_(false)
{
}

Channel::~Channel()
{
}

// 在TcpConnection建立得时候会调用
void Channel::tie(const std::shared_ptr<void> &obj)
{
    // weak_ptr 指向 obj
    tie_ = obj;
    tied_ = true;
}

// 通过该 Channel 所属的 EventLoop，调用 Epoller 对应的方法，注册 fd 的 events 事件
void Channel::update()
{    
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中，把当前的channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

// fd 得到 Epoller 通知以后，去处理事件
void Channel::handleEvent(Timestamp receiveTime)
{
    /**
     * TcpConnection::connectEstablished 会调用 channel_->tie(shared_from_this()) ; tied_ = true ;
     * 对于 TcpConnection::channel_ 就可以多一份强引用的保证，以免用户误删 TcpConnection 对象
     */
    if (tied_)
    {
        // 变成shared_ptr增加引用计数，防止误删
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        // guard为空情况，说明Channel的TcpConnection对象已经不存在了
    }
    else 
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据相应事件执行回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{    
    // 对端关闭事件
    // 当 TcpConnection 对应 Channel，通过 shutdown 关闭写端，epoll 触发EPOLLHUP
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        // 确认是否拥有回调函数
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    // 错误事件
    if (revents_ & (EPOLLERR))
    { 
        LOG_ERROR("the fd = %d" , this->fd()) ; 
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    // 读事件
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        LOG_DEBUG("channel have read events, the fd = %d" , this->fd()) ;
        if (readCallback_)
        {
            LOG_DEBUG("channel call the readCallback_(), the fd = %d" , this->fd()) ;
            readCallback_(receiveTime);
        }
    }

    // 写事件
    if (revents_ & EPOLLOUT)
    {
        LOG_DEBUG("channel have write events, the fd = %d" , this->fd()) ;
        if (writeCallback_)
        {
            LOG_DEBUG("channel call the writeCallback_(), the fd = %d" , this->fd()) ;
            writeCallback_();
        }
    }
}