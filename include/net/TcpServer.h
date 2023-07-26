#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <functional>
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>

#include "../base/noncopyable.h"
#include "../net/EventLoop.h"
#include "../net/EventLoopThreadPool.h"
#include "../net/Acceptor.h"
#include "../net/InetAddress.h"
#include "../net/Callback.h"
#include "../net/TcpConnection.h"

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
                const InetAddress &ListenAddr,
                const std::string &nameArg,
                Option option = kNoReusePort) ;
    ~TcpServer() ;

     // 设置回调函数(用户自定义的函数传入)
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

     // 设置底层subLoop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();
    
    EventLoop* getLoop() const { return loop_; }

    const std::string name() { return name_; }

    const std::string ipPort() { return ipPort_; }

private : 
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr> ;
    
    EventLoop *loop_;                                 // 用户定义的baseLoop
    const std::string ipPort_;                        // 传入的IP地址和端口号
    const std::string name_;                          // TcpServer名字
    std::unique_ptr<Acceptor> acceptor_;              // Acceptor对象负责监视
    std::shared_ptr<EventLoopThreadPool> threadPool_; // 线程池

    ConnectionCallback  connectionCallback_;        // 有新连接时的回调函数
    MessageCallback messageCallback_;               // 有读写消息时的回调函数
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成以后的回调函数

    ThreadInitCallback threadInitCallback_;         // loop线程初始化的回调函数
    std::atomic_int started_;                       // TcpServer 是否已经启动了

    int nextConnId_;                                // subloop_ 连接索引
    ConnectionMap connections_;                     // 保存所有 fd_name 对应的 TcpConnection 的连接
} ; 

#endif