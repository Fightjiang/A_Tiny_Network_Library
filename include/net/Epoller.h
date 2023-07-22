#ifndef Epoller_H
#define Epoller_H

#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>
#include "../base/noncopyable.h"
#include "../net/Channel.h"


class Epoller : noncopyable
{
public:
    using EventList = std::vector<epoll_event> ;
    using ChannelList = std::vector<Channel*> ;
    
    Epoller(EventLoop *Loop) ;
    ~Epoller() ;

    Timestamp epollWait(int timeoutMs, ChannelList *activeChannels) ;
    void updateChannel(Channel *channel) ;
    void removeChannel(Channel *channel) ; 
    bool hasChannel(Channel *channel) const; // 判断 channel是否注册到 poller当中

private:
    static const int kNew = -1  ; // 未添加在 Epoller 中
    static const int kAdded = 1 ; // 已添加在 Epoller 中
    static const int kDeleted = 2;// 已删除在 Epoller 中

    // 一次监听最大能返回事件数量
    static const int kInitEventListSize = 16; 

    // 填充活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道，本质是调用了epoll_ctl
    void update(int operation, Channel *channel);

    int epollfd_;           // epoll_create在内核创建空间返回的fd
    EventList events_;      // 用于存放epoll_wait返回的所有发生的事件的文件描述符

    EventLoop *ownerLoop_;  // 定义Poller所属的事件循环EventLoop
    using ChannelMap = std::unordered_map<int, Channel*> ;
    ChannelMap channels_ ;  // 储存 channel 的映射，（sockfd -> channel*）
};

#endif