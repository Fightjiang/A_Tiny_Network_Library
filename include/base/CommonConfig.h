#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

 
// #define SLEEP_MILLISECOND(ms)        \
//     std::this_thread::sleep_for(std::chrono::milliseconds(ms)); 

// #define SLEEP_SECOND(s)              \
//     std::this_thread::sleep_for(std::chrono::seconds(s));  

struct ConfigInfo{
    int  default_thread_size_ = 4 ;                                  // 默认开启主线程个数,因为本环境 CPU 是 4 核的
    int  max_thread_size_ = default_thread_size_ * 2 ;               // 最多线程个数
    int  max_thread_queue_size = 6 ;                                // 单个线程里面任务数超过 10 个，则放入公共线程池给其他线程处理
    int  pick_task_size = 3 ;                                        // 当本线程任务队列空了，尝试向公共队列取任务数量
    int  secondary_thread_ttl_  = 3 ;                                // 辅助线程 ttl , 单位为 s
    int  monitor_span_ = 3 ;                                         // 监控线程执行时间间隔，单位为 s
    bool fair_lock_enable_ = false ;                                 // 是否开启公平锁，则所有的任务都是从线程池的中获取。（非必要不建议开启，因为这样所有线程又要争抢一个任务了）
    const char * mysql_host = "localhost" ;                          // 数据库 IP 
    int mysql_port = 3306 ;                                          // 数据库端口
    const char * mysql_user = "root" ;                               // 数据库账号
    const char * mysql_pwd = "root123" ;                             // 数据库密码
    const char * mysql_dbName = "chatroom" ;                         // 使用的 database
    int mysql_maxConnCount = 16   ; 
    int mysql_minConnCount = 8   ;
    int mysql_timeOut = 2 * 1000 ;           // 想获得阻塞的连接，阻塞 2s 则返回空不再等待
    int mysql_maxIdleTime = 5 * 1000 ; // 空闲连接空闲了 5s
    // const char *server_IP = "127.0.0.1" ;                         // 服务器 Ip  
    const char *server_IP = "172.19.103.50" ;                        // 服务器 Ip  
    int server_port = 8080 ;                                         // 服务器端口
    //int timeoutS =  60 * 5 ;                                        // 是否超时断开不活跃连接，单位时间是秒
    int timeoutS = -1 ;                                               // 是否超时断开不活跃连接
    int server_max_fd = 4000 ;                                       // 服务器最大连接数量，因为我的一个进程能打开 file fd 默认设置最大只有 4096 ，再保留 96 个，可能用于其他用途，如 open                                              
    bool openLog = true ;                                            // 默认打开日志
    int logLevel = 0 ;                                               // 日志信息级别
    bool logWriteMethod = false ;                                    // true 异步写入，false 同步写入
    bool optLinger = true ;                                          // 优雅关闭：close() 之后等待一定时间，等套接字发送缓冲区中的数据发送完成
    int trigMode = 3 ;                                               // 采用的触发模式，0 水平触发；1 客户端 ET 服务端 LT ; 2  客户端 LT 服务端 ET; 3 客户端 ET 服务端 ET ; default = 3 ; 
}; 


#endif