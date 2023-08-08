#include "./http/HttpServer.h" 
#include "./http/HttpRequest.h"
#include "./http/HttpResponse.h"
#include "./base/CommonConfig.h"
#include "./log/Logging.h"
#include <fcntl.h>       // open
#include <sys/mman.h>    // mmap, munmap
#include <sys/stat.h> 

std::string getFileType(const HttpConfigInfo& httpConfig_ , const std::string &path_) {
    /* 判断文件类型 */
    std::string::size_type idx = path_.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if(httpConfig_.SUFFIX_TYPE.count(suffix) == 1) {
        return httpConfig_.SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

bool addResponseBody(HttpResponse* response , const std::string &filePath)
{
    struct stat mmFileStat_ = {0} ;
    bool file_exist = ::stat(filePath.data() , &mmFileStat_) == 0 ;
    if(!file_exist || S_ISDIR(mmFileStat_.st_mode)) { 
        LOG_ERROR("file %s not exist!!!" , filePath.data());
        response->setBody("404") ; 
        return false ;   
    }

    auto close_func = [](int* fd) {
        if (fd) {
            ::close(*fd);
            delete fd ;
            fd = nullptr ; 
        }
    };
    std::unique_ptr<int , decltype(close_func) > fd(new int(open(filePath.data(), O_RDONLY)) , close_func);
    if (*fd == -1) {
        LOG_ERROR("open file %s error !!!" ,  filePath.data()); 
        response->setBody("404") ; 
        return false ;   
    }
    
    auto munmap_func = [&mmFileStat_](char *data){
        if(data != MAP_FAILED){
            munmap(data , mmFileStat_.st_size) ; 
        }
    } ; 

    std::shared_ptr<char> dataFile(
            reinterpret_cast<char*>(mmap(nullptr, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE , *fd , 0)), 
            munmap_func
        ) ; 
    if(dataFile.get()  == MAP_FAILED) { 
        LOG_ERROR("mmap file %s error %d !!!" ,  filePath.data() , errno);
        response->setBody("404") ; 
        return false ;   
    }
    response->setBody(dataFile , mmFileStat_.st_size) ;  
    return true ;
}

void dealHttpRequest(const HttpRequest& request , HttpResponse* response) 
{ 
    std::string filePath = request.path() ;
    // 只是测试
    if (filePath == "/")
    {
        response->setStatusCode(HttpResponse::k200Ok);
        response->setStatusMessage("OK");
        response->setContentType("text/html");
        response->addHeader("Server", "Server Test");
        std::string now = Timestamp::now().toFormattedString();
        response->setBody("<html><head><title>This is title</title></head>"
            "<body><h1>Hello</h1>Now is " + now +
            "</body></html>");
    }
    else 
    {
        if(addResponseBody(response , request.getSrcDirPath() + filePath))
        {
            response->setStatusCode(HttpResponse::k200Ok);
            response->setStatusMessage("OK");
            response->setContentType(request.getFileType()) ;
            response->addHeader("Server", "Tiny WebServer");
        }
        else
        {
            response->setStatusCode(HttpResponse::k404NotFound);
            response->setStatusMessage("File Not Found");
            response->setCloseConnection(true);
        }
    }
}

int main() 
{
    ServerConfigInfo ServerConfig_ ; 
    EventLoop loop ; // main_loop 
    InetAddress addr(ServerConfig_.server_Port , ServerConfig_.server_IP) ; 
    HttpServer server(&loop , addr , "Http Server Test") ;  
    std::cout << addr.toIpPort() << std::endl; 
    server.setHttpCallback(dealHttpRequest) ;
    server.start() ; // 主要是启动 subLoop ，并且把 ServerFd 置于监听的状态
    loop.loop() ; // 启动主线程监听 ServerFd 上的可读事件（客户端连接 
    return 0 ;
}
