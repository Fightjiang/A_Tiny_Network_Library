#include "./http/HttpServer.h"
#include "./http/HttpRequest.h"
#include "./http/HttpResponse.h"

void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop *loop,
                       const InetAddress& listenAddr,
                       const std::string& name,
                       TcpServer::Option option)
        : server_(loop , listenAddr , name , option) , 
          httpCallback_(defaultHttpCallback)
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));

    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    
    server_.setThreadNum(4);
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        LOG_INFO("new Connection arrived") ;
    }
    else 
    {
        LOG_INFO("Connection closed") ;
    }
}

// 有消息到来时的业务处理
void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{ 
    std::unique_ptr<HttpRequest> requests(new HttpRequest);

#if 0
    // 打印请求报文
    std::string request = buf->GetBufferAllAsString();
    std::cout << request << std::endl;
#endif

    // 进行状态机解析
    // 错误则发送 BAD REQUEST 半关闭
    if (!requests->parseRequest(buf, receiveTime))
    {
        LOG_INFO("parseRequest failed!");
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    // 如果成功解析
    if (requests->gotAll())
    {
        LOG_INFO("parseRequest success!") ;
        onDealRequest(conn, requests->request());
        requests->reset();
    }
}

void HttpServer::onDealRequest(const TcpConnectionPtr& conn, const HttpRequest& request)
{
    const std::string& connection = request.getHeader("Connection");
    bool close = connection == "close" ||
        (request.version() == HttpRequest::kHttp10 && connection != "Keep-Alive"); 
   
    //  响应信息
    HttpResponse response(close);
    // httpCallback_ 由用户传入，怎么写响应体由用户决定 
    httpCallback_(request, &response);
    Buffer buf ; 
    response.appendHeaderToBuffer(&buf); 
    response.appendBodyToBuffer(&buf); 
    // LOG_INFO("bufStr = %s , buf = %d" , buf.GetBufferAllAsString().data() , buf.readableBytes()) ; 
    conn->send(&buf);

    if (response.closeConnection())
    {
        conn->shutdown();
    }
}