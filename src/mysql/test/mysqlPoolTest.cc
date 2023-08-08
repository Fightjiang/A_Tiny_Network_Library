#include <iostream>
#include "./mysql/ConnectionPool.h"
#include "./mysql/MysqlConn.h"
#include "./base/CommonConfig.h"
using namespace std ; 
// 1. 单线程: 使用/不使用连接池
// 2. 多线程: 使用/不使用连接池

ServerConfigInfo config_ ; 
// 非连接池
void op1(int begin, int end)
{
    for (int i = begin; i <= end; ++i)
    {
        MysqlConn conn;
        conn.connect(config_.mysql_user, config_.mysql_pwd, config_.mysql_dbName, config_.mysql_host);
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "insert into userTest values(%d, 'zhang san', 'passwd') ;", i);
        if(!conn.update(sql)) {
            break ;
        }
    }
}

// 连接池
void op2(ConnectionPool& pool, int begin, int end)
{
    for (int i = begin; i <= end; ++i)
    {
        shared_ptr<MysqlConn> conn = pool.getConnection();
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "insert into userTest values(%d, 'zhang san', 'passwd') ;", i);
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
#if 0
    // 非连接池, 单线程, 用时: 103519 毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op1(1, 1000);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "非连接池, 单线程, 用时: " << duration << " 毫秒 " << std::endl ; 
#else
    // 连接池, 单线程, 用时: 92432 毫秒
    ConnectionPool& pool = ConnectionPool::getConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op2(pool, 1, 1000);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "连接池, 单线程, 用时: " << duration << " 毫秒 " << std::endl ; 
#endif
}

// 多线程插入
void test2()
{
#if 0
    // 非连接池, 多线程, 用时:  37417 毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op1, 1, 200);
    std::thread t2(op1, 201, 400);
    std::thread t3(op1, 401, 600);
    std::thread t4(op1, 601, 800);
    std::thread t5(op1, 801, 1000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "非连接池, 多线程, 用时: " << duration << " 毫秒 " << std::endl ; 
#else
    // 连接池, 多线程, 用时:  37261 毫秒
    ConnectionPool& pool = ConnectionPool::getConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op2, std::ref(pool) ,   1, 200);
    std::thread t2(op2, std::ref(pool) , 201, 400);
    std::thread t3(op2, std::ref(pool) , 401, 600);
    std::thread t4(op2, std::ref(pool) , 601, 800);
    std::thread t5(op2, std::ref(pool) , 801, 1000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "连接池, 多线程, 用时: " << duration << " 豪秒 " << std::endl ; 
#endif
}

// 多线程查询
void test3()
{
#if 0
    // 非连接池, 多线程, 用时: 28 毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op3, 1, 200);
    std::thread t2(op3, 201, 400);
    std::thread t3(op3, 401, 600);
    std::thread t4(op3, 601, 800);
    std::thread t5(op3, 801, 1000); 
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "非连接池, 多线程, 用时: " << duration << " 毫秒 " << std::endl ; 
#else 
    // 连接池, 多线程, 用时: 2 毫秒
    ConnectionPool& pool = ConnectionPool::getConnectionPool();
     std::chrono::steady_clock::time_point begin =  std::chrono::steady_clock::now();
    std::thread t1(op4, std::ref(pool) ,   1, 200);
    std::thread t2(op4, std::ref(pool) , 201, 400);
    std::thread t3(op4, std::ref(pool) , 401, 600);
    std::thread t4(op4, std::ref(pool) , 601, 800);
    std::thread t5(op4, std::ref(pool) , 801, 1000); 
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end =  std::chrono::steady_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "连接池, 多单线程, 用时: " << duration << " 毫秒 " << std::endl ;  
#endif
}

// 查询测试
int query()
{
    MysqlConn conn;
    conn.connect(config_.mysql_user, config_.mysql_pwd, config_.mysql_dbName, config_.mysql_host);
    string sql = "insert into userTest values(1, 'zhang san', 'passwd')";
    bool flag = conn.update(sql);
    cout << "flag value:  " << flag << endl;

    sql = "select * from userTest";
    conn.query(sql);
    // 从结果集中取出一行
    while (conn.next())
    {
        // 打印每行字段值
        cout << conn.value(0) << ", "
            << conn.value(1) << ", "
            << conn.value(2) << endl;
    }
    return 0;
}

int main()
{
    test3() ; 
    return 0 ; 
}