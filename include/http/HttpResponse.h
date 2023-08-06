#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <string>
#include <string.h>
#include <memory>

class Buffer ; 
class HttpResponse
{
public:
    // 响应状态码
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };  

    explicit HttpResponse(bool close) : statusCode_(kUnknown), closeConnection_(close) { }

    void setStatusCode(HttpStatusCode code) { statusCode_ = code; } 
    void setStatusMessage(const std::string& message) { statusMessage_ = message; }   
    void setCloseConnection(bool on) { closeConnection_ = on; }  
    void setBody(const std::string& body)  { 
        std::shared_ptr<char> tmpBody(new char[body.size() + 1]) ; 
        ::strcpy(tmpBody.get(), body.data());  
        setBody(tmpBody , body.size()) ;
    } 
    void setBody(const std::shared_ptr<char>& body , size_t len) { body_ = body ; bodyLen_ = len ; }  
    void setContentType(const std::string& contentType) { addHeader("Content-Type", contentType); } 

    bool closeConnection() const { return closeConnection_; }
    void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }  
    void appendHeaderToBuffer(Buffer* output) const ;
    void appendBodyToBuffer(Buffer* output) const ;

private: 
    std::unordered_map<std::string, std::string> headers_;
    HttpStatusCode statusCode_; 
    std::string statusMessage_;
    std::shared_ptr<char> body_ ;
    size_t bodyLen_ ; 
    bool closeConnection_;
}; 


#endif