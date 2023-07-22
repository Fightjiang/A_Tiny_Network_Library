#include "./log/Logging.h"
#include "./net/Acceptor.h"
#include "./net/InetAddress.h"

#include <unistd.h> // ::close

static int createNonblocking()
{
    // SOCK_CLOEXEC 字段，避免子进程执行 exec 系统调用的时候关闭该文件描述符
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("listen socket create err %d" , errno) ; 
    }
    return sockfd;
}

// loop 是 main_loop 用户定义的
// ListenAddr 也是用户指定的
Acceptor::Acceptor(EventLoop *loop, const InetAddress &ListenAddr, bool reuseport) 
    : loop_(loop),
    acceptSocket_(createNonblocking()),
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false)
{
    LOG_DEBUG("Acceptor create nonblocking socket, [fd = %d ]" , acceptChannel_.fd() ) ;
    
    acceptSocket_.setReuseAddr(reuseport);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(ListenAddr);

    /**
     * TcpServer::start() => Acceptor.listen()
     * 有新用户的连接，需要执行一个回调函数，获得对应连接 fd ，并建立 TcpConnection 对象 。
     */
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));   
}

// 从 Epoller 中移除 acceptFd 
Acceptor::~Acceptor()
{    
    acceptChannel_.disableAll();    
    acceptChannel_.remove();       
}

// 表示正在监听
void Acceptor::listen()
{
    listenning_ = true ;
    acceptSocket_.listen() ; 
    acceptChannel_.enableReading() ;
}

// listenfd 有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr; // 保存新连接对应的 InetAddress 
    int connfd = acceptSocket_.accept(&peerAddr); // 接受新连接 
    if (connfd >= 0)
    {
        // TcpServer 中设置了对应的回调函数，轮询找到 subLoop 唤醒并分发当前的新客户端的Channel
        if (NewConnectionCallback_)
        { 
            NewConnectionCallback_(connfd, peerAddr); 
        }
        else
        {
            LOG_ERROR("no newConnectionCallback() function") ; 
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("accept() failed" ); 
        // 当前进程的fd已经用完了, 可以调整单个服务器的fd上限, 也可以分布式部署
        if (errno == EMFILE)
        {
            LOG_ERROR("sockfd reached limit") ;
        }
    }
}

