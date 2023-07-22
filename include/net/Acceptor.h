#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "Socket.h"
#include "Channel.h"

// 前置声明，可以不引用头文件暴露文件信息
class EventLoop;
class InetAddress;

/**
 * Acceptor 运行在 mainLoop 中
 * TcpServer发现 Acceptor 有一个新连接，则将此 Channel 分发给一个 subLoop
*/
class Acceptor : noncopyable
{
public : 
    // 接受新连接的会执行的回调函数
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)> ;
    Acceptor(EventLoop *loop, const InetAddress &ListenAddr, bool reuseport);
    ~Acceptor();

    // 主要是在 TcpServer 设置新连接到来，需要执行的回调函数，
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        NewConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }

    void listen() ;

private :

    void handleRead();

    EventLoop *loop_; // Acceptor用的就是用户定义的BaseLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback NewConnectionCallback_;
    bool listenning_; // 是否正在监听的标志
};

#endif