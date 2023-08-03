#include "./mysql/ConnectionPool.h"
#include "./log/Logging.h"


ConnectionPool& ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool;
    return pool;
}

ConnectionPool::ConnectionPool()
{ 
    loadConfigure() ; 
    for (int i = 0; i < minConnCount_; ++i)
    {
        addConnection();
        currentConnCount_++;
    }

    // 开启新线程执行任务
    std::thread producer(&ConnectionPool::produceConnection, this);
    std::thread recycler(&ConnectionPool::recycleConnection, this);
    
    // 设置线程分离，不阻塞在此处
    producer.detach();
    recycler.detach();
}

ConnectionPool::~ConnectionPool()
{
    // 释放队列里管理的MySQL连接资源
    while (!connectionQueue_.empty())
    {
        MysqlConn* conn = connectionQueue_.front();
        connectionQueue_.pop(); delete conn ;
        currentConnCount_--;
    }
}

void ConnectionPool::loadConfigure() 
{
    ConfigInfo config_ ; 
    ip_     = config_.mysql_host    ;  
    user_   = config_.mysql_user  ;
    passwd_ = config_.mysql_pwd ;
    dbName_ = config_.mysql_dbName ;
    port_   = config_.mysql_port  ;
    minConnCount_ = config_.mysql_minConnCount ;
    maxConnCount_ = config_.mysql_maxConnCount ;
    timeout_      = config_.mysql_timeOut ;
    maxIdleTime_  = config_.mysql_maxIdleTime ; 
}

void ConnectionPool::produceConnection()
{
    while (true)
    {
        std::unique_lock<std::mutex> locker(mutex_);
        // 条件变量检查是否是空的 , 为空则往下继续执行
        cond_.wait(locker , [&]{ return connectionQueue_.empty(); }) ;
        
        // 还没达到连接最大限制
        if (currentConnCount_ < maxConnCount_)
        {
            addConnection();  
            // 唤醒被阻塞的线程
            cond_.notify_all();
        } 
        else // 到达连接最大数量了，等待 1 秒再进行资源判断
        {
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }      
    }
}

// 销毁多余的数据库连接
void ConnectionPool::recycleConnection()
{
    while (true)
    {
        // 周期性的做检测工作，每 1s 执行一次
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        std::lock_guard<std::mutex> locker(mutex_);
        // 可能会有空闲连接等待太久
        while (connectionQueue_.size() > minConnCount_)
        {
            MysqlConn* conn = connectionQueue_.front();
            if (conn->getAliveTime() >= maxIdleTime_)
            {
                // 存在时间超过设定值则销毁
                connectionQueue_.pop(); delete conn;
                currentConnCount_--;
            }
            else
            {
                // 按照先进先出顺序，前面的没有超过后面的肯定也没有
                break;
            }
        }
    }
}

void ConnectionPool::addConnection()
{
    MysqlConn* conn = new MysqlConn;
    conn->connect(user_, passwd_, dbName_, ip_, port_);
    conn->refreshAliveTime();    // 刷新起始的空闲时间点
    connectionQueue_.push(conn); // 记录新连接
    currentConnCount_++;
}

// 获取连接
std::shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> locker(mutex_);
    // while 避免虚假唤醒
    while (connectionQueue_.empty())
    {
        // 如果为空，需要阻塞一段时间，等待新的可用连接
        if (std::cv_status::timeout == cond_.wait_for(locker, std::chrono::milliseconds(timeout_)))
        {
            // std::cv_status::timeout 表示超时
            if (connectionQueue_.empty())
            {
                LOG_INFO("get connection timeout ...");
                return nullptr;
            }
        }
    } 
    
    // 非常巧妙的一个操作，使用共享智能指针并规定其删除器
    // 规定销毁后调用删除器，在互斥的情况下更新空闲时间并加入数据库连接池
    std::shared_ptr<MysqlConn> connptr(connectionQueue_.front(), 
        [this](MysqlConn* conn) {
            std::lock_guard<std::mutex> locker(mutex_);
            conn->refreshAliveTime();
            connectionQueue_.push(conn);
        }) ;

    connectionQueue_.pop();
    if (connectionQueue_.empty()) {
        //没有可用连接了，通知生产线程去生产
        cond_.notify_all();
    }
    
    return connptr;
}

