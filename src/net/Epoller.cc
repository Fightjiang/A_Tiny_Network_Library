#include "./net/Epoller.h"
#include <string.h>

Epoller::Epoller(EventLoop *loop) :
        ownerLoop_(loop), 
        epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
        events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create() error: %d" , errno) ;
    }
}

Epoller::~Epoller()
{
    ::close(epollfd_);
}

Timestamp Epoller::epollWait(int timeoutMs, ChannelList *activeChannels)
{
    size_t numEvents = ::epoll_wait(epollfd_, &(*events_.begin()), 
                                    static_cast<int>(events_.size()), 
                                    timeoutMs) ;
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    // 有事件产生
    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels); // 填充活跃的 channels
        // 对 events_ 进行扩容操作
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    // 超时
    else if (numEvents == 0)
    {
        LOG_DEBUG("timeout!");
    }
    // 出错
    else
    {
        // 不是终端错误
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() failed");
        }
    }
    return now;
}

/**
 * Channel::update => EventLoop::updateChannel => Epoller::updateChannel
 * Channel::remove => EventLoop::removeChannel => Epoller::removeChannel
 */
void Epoller::updateChannel(Channel *channel)
{
    
    const int index = channel->index(); // 获取参数channel在epoll的状态
    
    // 未添加状态和已删除状态都有可能会被再次添加到epoll中
    if (index == kNew || index == kDeleted)
    {
        // 添加到键值对 
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel; 
        }
        // else index == kDeleted
        
        // 修改channel的状态，此时是已添加状态
        channel->set_index(kAdded);
        // 向epoll对象加入channel
        update(EPOLL_CTL_ADD, channel);
    }
    else // kAdded , channel已经在 Epoller 上注册过
    {
        // 没有感兴趣事件，说明可以从 epoll 对象中删除该 channel 了
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        // 还有事件，则 update 修改 channel 状态
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoller::removeChannel(Channel *channel)
{
    // 从Map中删除
    int fd = channel->fd();
    channels_.erase(fd); 
 
    int index = channel->index();
    if (index == kAdded)
    {
        // 如果此fd已经被添加到Poller中，则还需从epoll对象中删除
        update(EPOLL_CTL_DEL, channel);
    }
    // 重新设置channel的状态为未被Poller注册
    channel->set_index(kNew);
}

// 判断参数channel是否在当前poller当中
bool Epoller::hasChannel(Channel *channel) const
{ 
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

// 真正的更新状态
void Epoller::update(int operation, Channel *channel)
{
    epoll_event event;
    ::memset(&event, 0, sizeof(event));

    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl() del error: %d" , errno) ;
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error: %d", errno) ;
        }
    }
}

// 填写活跃的连接到 activeChannels 中，在对应的线程中执行 channel 对应的回调函数
void Epoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    { 
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}