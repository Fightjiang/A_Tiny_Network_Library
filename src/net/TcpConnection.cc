#include <functional> 
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "./net/TcpConnection.h"
#include "./log/Logging.h"
#include "./net/Socket.h"
#include "./net/Channel.h"
#include "./net/EventLoop.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    // 如果传入EventLoop没有指向有意义的地址则出错
    // 正常来说在 TcpServer::start 这里就生成了新线程和对应的EventLoop
    if (loop == nullptr)
    {
        LOG_FATAL("mainLoop is null!") ;
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) // 64M 避免发送太快对方接受太慢
{
     // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生了 channel会回调相应的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::create[ %s ] at fd = %d " , name_.c_str() , sockfd) ;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::create[ %s ] at fd = %d " , name_.c_str() , channel_->fd()) ; 
}

// 发送数据
void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf) ;
        }
        else
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop ,
                this,
                buf)) ;
        }
    }
}

void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->retrieveAllAsString()); 
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this , buf->retrieveAllAsString())) ;
        }
    }
}


// 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，故设置了水位回调
void TcpConnection::sendInLoop(const std::string& message)
{
    const char *data = message.data() ; 
    size_t len = message.size() ; 
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false; // 是否对端已经关闭了

    // 之前调用过connection得shutdown，不能再进行发送了
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing") ;
        return ;
    }

    // channel第一次写数据，且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            // 判断有没有一次性写完
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 写完执行 writeCompleteCallback 回调
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else // nwrote = 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop , maybe peer already close");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE
                {
                    faultError = true;
                }
            }
        }

    }

    // 说明一次性并没有发送完数据，剩余数据需要保存到缓冲区中，且需要改channel注册监听写事件
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ 
        && oldLen < highWaterMark_ 
        && highWaterMarkCallback_)
        {
            // 发送缓冲区已经超过高水位了，执行高水位回调函数
            loop_->queueInLoop(std::bind(
                highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 这里一定要注册channel的写事件 否则当文件描述符 fd 可写时，epoller 不会给 channel 通知执行可写的回调函数
            channel_->enableWriting(); 
        }
    }
}

// 关闭连接 
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    // 说明当前 outputBuffer_ 的数据全部向外发送完成
    if (!channel_->isWriting()) 
    {
        socket_->shutdownWrite();
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected); // 建立连接，设置一开始状态为连接态
    // tie 防止 channel 在执行回调函数的时候，TcpConnection 已经被删除了
    channel_->tie(shared_from_this());
    // 向 epoller 注册 channel 的EPOLLIN读事件
    channel_->enableReading(); 
    // 新连接建立 执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        // 把 channel 的所有感兴趣的事件从 epoller 中删除掉
        channel_->disableAll(); 
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把 channel 从 epoller 中删除掉
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0 ; 
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读事件发生，调用用户传入的回调操作
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        // 没有数据，说明客户端关闭连接
        handleClose();
    }
    else
    {
        // 出错情况
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead() failed") ;
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        // 正确读取数据
        if (n > 0)
        {
            outputBuffer_.retrieve(n) ; 
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting() ;
                // 调用用户自定义的写完数据处理函数
                if (writeCompleteCallback_)
                {
                    // 唤醒 loop_ 对应得 thread 线程，执行写完成事件回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite() failed");
        }
    }
    else // state_不为写状态
    {
        LOG_ERROR("TcpConnection fd= %d  is down, no more writing " , channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    setState(kDisconnected);    // 设置状态为关闭连接状态
    channel_->disableAll();     // 注销Channel所有感兴趣事件

    // 关闭连接 , 为什么还要继续增加一个引用计数的智能指针呢？ 
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);   
    closeCallback_(connPtr);        
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0 ; 
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen))
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("cpConnection::handleError name: %s  - SO_ERROR: %d ", name_.c_str() , err) ;
}