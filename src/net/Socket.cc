#include "./net/Socket.h"
#include "./log/Logging.h"
#include "./net/InetAddress.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd: %d fail" , sockfd_) ; 
    }
}

void Socket::listen() 
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd: %d fail" , sockfd_) ;
    }
}

int Socket::accept(InetAddress *peeraddr) 
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) 
    {
        peeraddr->setSockAddr(addr) ; 
    }
    else 
    {
        LOG_ERROR("accept4() failed") ;
    }
    return connfd ; 
}

// 关闭写端，但是可以接受客户端数据
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error") ;
    }
}

// 是否启动 Nagle 算法
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); 
}

// 是否设置地址复用，可以重用 Time-wait 阶段的端口
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); 
}

// 是否设置可以绑定同一个地址和端口，内核升级后才可以设置的。
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); 
}

// 是否设置在TCP 套接字上启用心跳机制。 
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); 
}