#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "../base/Timestamp.h"
#include "../base/CommonConfig.h"
#include <unordered_map>

class Buffer;

class HttpRequest 
{
public:
    enum Method { kInvalid, kGet, kPost, kHead, kPut, kDelete };
    enum Version { kUnknown, kHttp10, kHttp11 };    
    
    // HTTP请求状态
    enum HttpRequestParseState
    {
        kExpectRequestLine, // 解析请求行状态
        kExpectHeaders,     // 解析请求头部状态
        kExpectBody,        // 解析请求体状态
        kGotAll,            // 解析完毕状态
    };
    
    HttpRequest() : method_(kInvalid), version_(kUnknown) , state_(kExpectRequestLine) { }

    void setVersion(Version v)  { version_ = v; }
    Version version() const     { return version_; }
    
    const char* versionString() const
    {
        const char* result = "Unknown Http Version";
        switch(version_)
        {
          case kHttp10: result = "HTTP/1.0"; break;
          case kHttp11: result = "HTTP/1.1"; break; 
          default:
            break;
        }
        return result;
    } 

    // 目前只支持 GET 和 POST 请求
    bool setMethod(const char *start, const char *end)
    {
        std::string m(start, end);
        if (m == "GET") { method_ = kGet; }
        else if (m == "POST") { method_ = kPost; }
        else if (m == "HEAD") { method_ = kHead; }
        else if (m == "PUT")  { method_ = kPut;  }
        else if (m == "DELETE") { method_ = kDelete; }
        else { method_ = kInvalid; }
        // 判断method_是否合法
        return method_ != kInvalid;
    }

    Method method() const { return method_; }

    const char* methodString() const
    {
        const char* result = "UNKNOWN";
        switch(method_)
        {
          case kGet:  result = "GET"; break;
          case kPost: result = "POST"; break;
          case kHead: result = "HEAD"; break;
          case kPut:  result = "PUT"; break;
          case kDelete: result = "DELETE"; break;
          default:
            break;
        }
        return result;
    } 

    void setPath(const char *start, const char *end) { path_.assign(start, end);}

    const std::string path() const 
    {
        if((httpConfig_.DEFAULT_HTML.count(path_) != 0)) {
            return path_ + ".html" ; 
        }
        return path_; 
    }

    void setQuery(const char *start, const char *end) { query_.assign(start, end); }

    const std::string& query() const { return query_; }

    void setReceiveTime(Timestamp t)  {  receiveTime_ = t;  }

    Timestamp receiveTime() const { return receiveTime_; }
    
    // colon 冒号(:)
    void addHeader(const char *start, const char *colon, const char *end)
    {
        std::string field(start, colon);
        ++colon;
        // 跳过空格
        while (colon < end && isspace(*colon))
        {
            ++colon;
        }
        std::string value(colon, end);
        // value丢掉后面的空格，通过重新截断大小设置
        while (!value.empty() && isspace(value[value.size()-1]))
        {
          value.resize(value.size()-1);
        }
        headers_[field] = value;
    }

    // 获取请求头部的对应值
    std::string getHeader(const std::string &field) const
    {
        std::string result;
        auto it = headers_.find(field);
        if (it != headers_.end())
        {
            result = it->second;
        }
        return result;
    }

    const std::unordered_map<std::string, std::string>& headers() const
    {
        return headers_;
    }

    const std::unordered_map<std::string, std::string>& postData() const
    {
        return postData_;
    }

    // void swap(HttpRequest &rhs)
    // {
    //     std::swap(method_, rhs.method_);
    //     std::swap(version_, rhs.version_);
    //     path_.swap(rhs.path_);
    //     query_.swap(rhs.query_);
    //     std::swap(receiveTime_, rhs.receiveTime_);
    //     headers_.swap(rhs.headers_);
    // }

    // 解析请求
    bool processRequestLine(const char *begin, const char *end);
    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    bool gotAll() const { return state_ == kGotAll; }
    // 重置 HttpRequest 状态 
    void reset()
    {
        state_   = kExpectRequestLine ;  
        method_  = kInvalid ;
        version_ = kUnknown ;
        path_    = "" ; 
        query_   = "" ;
        receiveTime_ = Timestamp::invalid() ;
        headers_.clear() ;  
    }

    const HttpRequest& request() const { return *this; }
    HttpRequest& request() { return *this; }
    
    // 新增的一些功能 
    std::string getFileType() const{
        /* 判断文件类型 */
        std::string filePath = this->path() ; 
        std::string::size_type idx = filePath.find_last_of('.');
        if(idx == std::string::npos) {
            return "text/plain";
        }
        std::string suffix = filePath.substr(idx);
        if(httpConfig_.SUFFIX_TYPE.count(suffix) == 1) {
            return httpConfig_.SUFFIX_TYPE.find(suffix)->second;
        }
        return "text/plain";
    }

    bool isUpgradeWebSocket() const {
        if(headers_.find("Connection") != headers_.end() && 
            headers_.find("Upgrade") != headers_.end() && 
            headers_.find("Sec-WebSocket-Key") != headers_.end() ) {

            return headers_.find("Connection")->second == "keep-Upgrade" || 
                    headers_.find("Upgrade")->second == "websocket" ; 
        
        }
        return false ; 
    }

    std::string getSrcDirPath() const {
        return httpConfig_.srcDir ; 
    }

    

private: 
    
    Method method_;                                         // 请求方法
    Version version_;                                       // 协议版本号
    HttpRequestParseState state_;                           // 解析请求行的当前状态
    std::string path_;                                      // 请求路径
    std::string query_;                                     // 询问参数
    Timestamp receiveTime_;                                 // 请求时间
    std::unordered_map<std::string, std::string> headers_;  // 请求头部列表
    std::unordered_map<std::string, std::string> postData_; // Post 请求数据
    HttpConfigInfo httpConfig_ ;                           // 处理 Http 请求需要用到的文件参数
};

#endif