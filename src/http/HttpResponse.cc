#include "./http/HttpResponse.h"
#include "./net/Buffer.h"

void HttpResponse::appendHeaderToBuffer(Buffer* output) const 
{
    char buf[32];
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", statusCode_ , statusMessage_.data());
    output->append(buf) ; 

    if (closeConnection_)
    {
        output->append("Connection: close\r\n");
    }
    else
    {
        // snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
        // output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    for (const auto &header : headers_) 
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }
    // output->append("\r\n");
    // output->append(body_) ;
}

void HttpResponse::appendBodyToBuffer(Buffer* output) const 
{
    if(body_ != nullptr)
    {
        output->append("Content-Length: " + std::to_string(bodyLen_));
        output->append("\r\n\r\n");
        output->append(body_.get() , bodyLen_) ;
    }
    else 
    {
        output->append("\r\n");
    }
}