#include "./http/HttpRequest.h"
#include "./net/Buffer.h"
#include <iostream>
#include <string>

void test_parse_http(){

    std::string http_head1 = "GET / HTTP/1.1\r\nUser-Agent: curl/7.16.3 libcurl/7.16.3 OpenSSL/0.9.7l zlib/1.2.3\r\nHost: www.example.com\r\nAccept-Language: en, mi\r\n" ; 
    std::string http_head2 = "POST /login HTTP/1.1\r\nUser-Agent: curl/7.16.3 libcurl/7.16.3 OpenSSL/0.9.7l zlib/1.2.3\r\nHost: www.example.com\r\nContent-Length: 31\r\nContent-Type: application/x-www-form-urlencoded\r\nAccept-Language: en, mi\r\n\r\nusername=test4&password=test%40" ; 
    
    HttpRequest request_ ; 
    Timestamp now = Timestamp::now() ; 
    Buffer* curBuffer = new Buffer() ; 
    curBuffer->append(http_head2) ; 
    request_.parseRequest(curBuffer , now) ; 
    std::cout<< "method = " << request_.methodString() << 
                " path = "  << request_.path() << 
                " version = " << request_.version() << std::endl ; 

    const std::unordered_map<std::string,std::string>& header = request_.headers() ; 
    for(auto iter : header) {
        std::cout << iter.first << ":" << iter.second << std::endl ;
    }
    
    const std::unordered_map<std::string,std::string>& postData = request_.postData() ; 
    for(auto iter : postData) {
        std::cout << iter.first << ":" << iter.second << std::endl ;
    }

    std::cout << curBuffer->readableBytes() << std::endl ;
}

int main()
{
    test_parse_http() ; 
    return 0 ; 
}