#include <iostream>
#include "./mysql/ConnectionPool.h"
#include "./mysql/MysqlConn.h"
#include "./base/CommonConfig.h"
using namespace std ; 
// 1. 单线程: 使用/不使用连接池
// 2. 多线程: 使用/不使用连接池

ConfigInfo config_ ; 
// 非连接池
void op1(int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        MysqlConn conn;
        conn.connect(config_.mysql_user, config_.mysql_pwd, config_.mysql_dbName, config_.mysql_host);
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "insert into userTest values(%d, 'zhang san', 'passwd') ;", i + 1);
        if(!conn.update(sql)) {
            break ;
        }
    }
}

// 连接池
void op2(ConnectionPool& pool, int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        shared_ptr<MysqlConn> conn = pool.getConnection();
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "insert into userTest values(%d, 'zhang san', 'passwd') ;", i + 1);
        if(!conn->update(sql)) {
            break ;
        }
    }
}

// 非连接池查询
void op3(int begin, int end)
{
    MysqlConn conn;
    conn.connect(config_.mysql_user, config_.mysql_pwd, config_.mysql_dbName, config_.mysql_host);
    char sql[1024] = { 0 };
    snprintf(sql, sizeof(sql), "select * from userTest where id between %d and %d ;" , begin , end);
    conn.query(sql);
}

// 连接池查询
void op4(ConnectionPool& pool, int begin, int end)
{
    shared_ptr<MysqlConn> conn = pool.getConnection();
    char sql[1024] = { 0 };
    snprintf(sql, sizeof(sql), "select * from userTest where id between %d and %d ;" , begin , end);
    conn->query(sql); 
}

// 单线程插入
void test1()
{
#if 1
    // 非连接池, 单线程, 用时: 34127689958 纳秒, 34127 毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op1(0, 10);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "非连接池, 单线程, 用时: " << duration << " 秒 " << std::endl ; 
#else
    // 连接池, 单线程, 用时: 19413483633 纳秒, 19413 毫秒
    ConnectionPool& pool = ConnectionPool::getConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op2(pool, 0, 5000);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock:::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "连接池, 单线程, 用时: " << duration << " 秒 " << std::endl ; 
#endif
}

// 多线程插入
void test2()
{
#if 0
    // 非连接池, 多线程, 用时: 15702495964 纳秒, 15702 毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op1, 0, 1000);
    std::thread t2(op1, 1000, 2000);
    std::thread t3(op1, 2000, 3000);
    std::thread t4(op1, 3000, 4000);
    std::thread t5(op1, 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "非连接池, 多线程, 用时: " << duration << " 秒 " << std::endl ; 
#else
    // 连接池, 多线程, 用时: 6076443405 纳秒, 6076 毫秒
    ConnectionPool& pool = ConnectionPool::getConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op2, std::ref(pool) , 0, 1000);
    std::thread t2(op2, std::ref(pool) , 1000, 2000);
    std::thread t3(op2, std::ref(pool) , 2000, 3000);
    std::thread t4(op2, std::ref(pool) , 3000, 4000);
    std::thread t5(op2, std::ref(pool) , 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "连接池, 多线程, 用时: " << duration << " 秒 " << std::endl ; 
#endif
}

// 多线程查询
void test3()
{
#if 1
    // 非连接池, 多线程, 用时: 15702495964 纳秒, 15702 毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op3, 0, 1000);
    std::thread t2(op3, 1000, 2000);
    std::thread t3(op3, 2000, 3000);
    std::thread t4(op3, 3000, 4000);
    std::thread t5(op3, 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "非连接池, 多线程, 用时: " << duration << " 秒 " << std::endl ; 
#else 
    // 连接池, 多线程, 用时: 6076443405 纳秒, 6076 毫秒
    ConnectionPool& pool = ConnectionPool::getConnectionPool();
     std::chrono::steady_clock::time_point begin =  std::chrono::steady_clock::now();
    std::thread t1(op4, std::ref(pool) , 0, 1000);
    std::thread t2(op4, std::ref(pool) , 1000, 2000);
    std::thread t3(op4, std::ref(pool) , 2000, 3000);
    std::thread t4(op4, std::ref(pool) , 3000, 4000);
    std::thread t5(op4, std::ref(pool) , 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end =  std::chrono::steady_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "连接池, 多单线程, 用时: " << duration << " 秒 " << std::endl ;  
#endif
}

// 查询测试
int query()
{
    MysqlConn conn;
    conn.connect("root", "123456", "test", "127.0.0.1");
    string sql = "insert into user values(1, 'zhang san', '221B')";
    bool flag = conn.update(sql);
    cout << "flag value:  " << flag << endl;

    sql = "select * from user";
    conn.query(sql);
    // 从结果集中取出一行
    while (conn.next())
    {
        // 打印每行字段值
        cout << conn.value(0) << ", "
            << conn.value(1) << ", "
            << conn.value(2) << ", "
            << conn.value(3) << endl;
    }
    return 0;
}

int main()
{
    test1() ; 
    return 0 ; 
}