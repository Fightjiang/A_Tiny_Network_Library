#include "./http/HttpRequest.h"
#include "./net/Buffer.h"
#include "./log/Logging.h"
/*
解析请求行
GET /text.html HTTP/1.1
Host: 127.0.0.1
Connection: Keep-Alive
Accept-Language: zh-cn
*/
bool HttpRequest::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');

    // 不是最后一个空格，并且成功获取了method并设置到request_
    if (space != end && this->setMethod(start, space))
    {
        // 跳过空格
        start = space+1;
        // 继续寻找下一个空格
        space = std::find(start, end, ' ');
        if (space != end)
        {
            // 查看是否有请求参数
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                // 设置访问路径
                this->setPath(start, question);
                // 设置访问变量
                this->setQuery(question, space);
            }
            else
            {
                this->setPath(start, space);
            }
            start = space+1;
            // 获取最后的http版本
            succeed = (end-start == 8 && std::equal(start, end-1, "HTTP/1."));
            if (succeed)
            {
                if (*(end-1) == '1')
                {
                    this->setVersion(HttpRequest::kHttp11);
                }
                else if (*(end-1) == '0')
                {
                    this->setVersion(HttpRequest::kHttp10);
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }  
    return succeed;
}

// 状态机解析 HTTP 请求
bool HttpRequest::parseRequest(Buffer* buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)
  {
    if (state_ == kExpectRequestLine)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        ok = this->processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          this->setReceiveTime(receiveTime);
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        const char* colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          this->addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          // empty line, end of header 
          if(this->method_ == kGet) {
            state_ = kGotAll;
            hasMore = false;
          } else if(this->method_ == kPost){
            state_ = kExpectBody;    
          }
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)
    {
      const char* crlf = buf->findCRLF();
      // 只解析 POST 请求的格式 application/x-www-form-urlencoded
      if(this->getHeader("Content-Type") == "application/x-www-form-urlencoded") 
      {
        auto ConverHex = [](const char ch) -> int {
            if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
            if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
            return ch - '0' ;
        } ; 
        size_t postLen = stol(headers_["Content-Length"]) ;
        const char* strBegin = buf->peek() ; 
        std::string key, value ;
        for(int i = 0 , flag = 1 ; i <= postLen ; ++i) { 
            if(i == postLen || strBegin[i] == '&' || strBegin[i] == '=') {
                if(i == postLen || strBegin[i] == '&'){
                    postData_[key] = value ; 
                    LOG_DEBUG("Post key:%s, value:%s", key.data(), value.data());
                    key.clear() ; value.clear() ;
                }
                flag *= -1 ; continue ; 
            } 
            char ch = *(strBegin + i) ;
            if(ch == '+'){
                ch = ' ' ; 
            }else if(ch == '%'){
                ch = static_cast<char>(ConverHex(*(strBegin + i + 1)) * 16 + ConverHex(*(strBegin + i + 2))) ; 
                i += 2 ;
            } 
            if(flag == 1) key.push_back(ch) ; 
            if(flag == -1) value.push_back(ch) ; 
        }
      }
      else 
      {
        LOG_INFO("Other post request format %s " , this->getHeader("Content-Type").data()) ; 
      }
      state_ = kGotAll;
      hasMore = false;
      buf->retrieveUntil(crlf + 2) ;
    }
  }
  return ok;
}
