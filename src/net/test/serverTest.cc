#include "./net/TcpServer.h"
#include <iostream>

class EchoServer
{
public : 
    EchoServer(EventLoop *loop , InetAddress &addr , std::string name) 
        : server_(loop, addr, name), loop_(loop) 
    {
        // 这个回调函数会一直绑定到 TcpConnection 类中，直到 TcpServer main_loop 监听到可连接事件后
        // 执行对应的 TcpConnection::connectEstablished 函数，再执行对应用户设置的 connection 
        server_.setConnectionCallback(std::bind(&EchoServer::connection , this , std::placeholders::_1)) ;     
        
        // 这个回调函数会一直绑定到 connfd 对应的 Channel 中，当 subLoop 监听到该文件描述符可读之后
        // 执行对应的 handleRead() 回调函数读取完数据，然后执行用户设置的 messageCallback 回调函数
        server_.setMessageCallback(std::bind(&EchoServer::message , this , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3)) ; 

        // 设置 subThread 线程的数量
        server_.setThreadNum(4) ; 
    }

    void start() 
    {
        server_.start() ; 
    }
    

private : 

    // 建立连接 或者 断开的回调
    void connection(const TcpConnectionPtr &conn) 
    {
        if (conn->connected())
        {
            LOG_INFO("conn up: %s", conn->peerAddress().toIpPort().data());
        }
        else 
        {
            LOG_INFO("conn down: %s", conn->peerAddress().toIpPort().data());
        }
    }

    // 可读事件回调
    void message(const TcpConnectionPtr &conn , Buffer *buffer , Timestamp time)
    {
        conn->send(buffer); 
        // conn->shutdown()  ; // 主动关闭写端，等待客户端关闭
    }

    EventLoop *loop_  ;
    TcpServer server_ ;
} ; 

int main() {
    EventLoop loop ; // main_loop 
    InetAddress addr(8080) ; 
    EchoServer server(&loop , addr , "Echo Server Test") ; 
    server.start() ; // 主要是启动 subLoop ，并且把 ServerFd 置于监听的状态
    loop.loop() ; // 启动主线程监听 ServerFd 上的可读事件（客户端连接
    return 0 ; 
}