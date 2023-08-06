#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "../net/TcpServer.h"
#include "../base/noncopyable.h"
#include "../log/Logging.h" 

class HttpRequest;
class HttpResponse;
class HttpServer : noncopyable
{
public:
    using HttpCallback = std::function<void (const HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop *loop,
            const InetAddress& listenAddr,
            const std::string& name,
            TcpServer::Option option = TcpServer::kNoReusePort);
    
    EventLoop* getLoop() const { return server_.getLoop(); }

    void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }

    void start() { server_.start() ; }

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr &conn,
                    Buffer *buf,
                    Timestamp receiveTime);
    void onDealRequest(const TcpConnectionPtr&, const HttpRequest&);

    TcpServer server_;
    HttpCallback httpCallback_;
};

#endif  