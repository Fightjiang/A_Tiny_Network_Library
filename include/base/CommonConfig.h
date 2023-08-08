#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <unordered_map>
#include <unordered_set>
#include <string>
 
// #define SLEEP_MILLISECOND(ms)        \
//     std::this_thread::sleep_for(std::chrono::milliseconds(ms)); 

// #define SLEEP_SECOND(s)              \
//     std::this_thread::sleep_for(std::chrono::seconds(s));  

struct ServerConfigInfo{
    const char * mysql_host = "localhost" ;                          // 数据库 IP 
    int mysql_port = 3306 ;                                          // 数据库端口
    const char * mysql_user = "root" ;                               // 数据库账号
    const char * mysql_pwd = "root123" ;                             // 数据库密码
    const char * mysql_dbName = "chatroom" ;                         // 使用的 database
    int mysql_maxConnCount = 16   ; 
    int mysql_minConnCount = 8   ;
    int mysql_timeOut = 2 * 1000 ;                                   // 想获得阻塞的连接，阻塞 2s 则返回空不再等待
    int mysql_maxIdleTime = 5 * 1000 ;                               // 空闲连接空闲了 5s
    const char *server_IP = "127.0.0.1" ;                            // 服务器 Ip  
    int server_Port = 8080 ;                                         // 服务器端口
}; 

struct HttpConfigInfo { 
    
    std::string srcDir = "/home/lec/File/New_WebServer/resources" ;  // 服务器文件所在的地址
    std::string jwtSecret = "Chatroom" ;                             // JWT 中的密钥设置
    int jwtExpire = 60*60   ;                                        // JWT 中的 token 过期时间 1 小时 , 单位是秒
    
    std::unordered_map<std::string, std::string> SUFFIX_TYPE = {
        { ".html",  "text/html" },
        { ".xml",   "text/xml" },
        { ".xhtml", "application/xhtml+xml" },
        { ".txt",   "text/plain" },
        { ".rtf",   "application/rtf" },
        { ".pdf",   "application/pdf" },
        { ".word",  "application/nsword" },
        { ".png",   "image/png" },
        { ".gif",   "image/gif" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".au",    "audio/basic" },
        { ".mpeg",  "video/mpeg" },
        { ".mpg",   "video/mpeg" },
        { ".avi",   "video/x-msvideo" },
        { ".gz",    "application/x-gzip" },
        { ".tar",   "application/x-tar" },
        { ".css",   "text/css "},
        { ".js",    "text/javascript "},
    };
    std::unordered_map<int, std::string> CODE_STATUS = {
        { 200, "OK" },
        { 400, "Bad Request" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
    }; 
    std::unordered_map<int, std::string> CODE_PATH = {
        { 400, "/400.html" },
        { 403, "/403.html" },
        { 404, "/404.html" },
    };
    std::unordered_set<std::string> DEFAULT_HTML{
            "/index", "/register", "/login", "/welcome"
             "/chat", "/video", "/picture", "/websocket"
    } ;
    std::unordered_map<std::string, int> DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  
    };
} ; 

#endif