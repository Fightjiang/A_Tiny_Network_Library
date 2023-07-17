#include "./log/Logging.h"
#include "./log/AsyncLogging.h"
#include <chrono>
#include <thread>

void test_Logging()
{
    LOG_DEBUG("debug") ;
    LOG_INFO("info");
    LOG_WARN("warn");
    LOG_ERROR("error");
    // 注意不能轻易使用 LOG_FATAL, LOG_SYSFATAL, 会导致程序abort
    const int n = 10;
    for (int i = 0; i < n; ++i) {
        LOG_INFO("Hello , test_Logging , %d  abc...xyz" , i);
    }
}

void test_AsyncLogging()
{
    const int n = 1024;
    for (int i = 0; i < n; ++i) {
        LOG_INFO("Hello , test_AsyncLogging , %d  abc...xyz" , i);
    }
}

int main()
{
    // 前端没有问题了
    // LOG_INFO("test test") ; 

    AsyncLogging &log = AsyncLogging::Instance() ; 
    Logger::setOutput([&](const char* msg, int len){
        log.append(msg , len) ; 
    }); 
    test_Logging() ;  
    
    // // 阻塞延迟 6 秒钟
    std::chrono::seconds duration(6);
    std::this_thread::sleep_for(duration);
    std::cout<< "log.stop()" << std::endl ; 
    log.stop() ; 
    std::cout<< "over log.stop()" << std::endl ;
    return 0;
}