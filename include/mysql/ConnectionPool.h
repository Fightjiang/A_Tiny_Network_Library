#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "../mysql/MysqlConn.h"
#include "../base/noncopyable.h"
#include "../base/CommonConfig.h"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

class ConnectionPool : noncopyable
{
public:
    static ConnectionPool& getConnectionPool();
    std::shared_ptr<MysqlConn> getConnection();
    

private:
    ConnectionPool(); 
    ~ConnectionPool();
    
    void loadConfigure() ; 
    void produceConnection();
    void recycleConnection();
    void addConnection();

    // TODO:加上文件路径
    // std::string filePath_;
    std::string ip_;
    std::string user_;
    std::string passwd_;
    std::string dbName_;
    unsigned short port_;
    int minConnCount_;
    int maxConnCount_;
    int currentConnCount_;
    int timeout_;
    int maxIdleTime_;
    std::queue<MysqlConn*> connectionQueue_; // 连接池队列
    std::mutex mutex_; 
    std::condition_variable cond_;
};


#endif