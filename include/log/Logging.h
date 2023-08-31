#ifndef LOGGING_H
#define LOGGING_H

#include "../base/FixedBuffer.h"
#include "../base/Timestamp.h"
#include <stdarg.h> 
#include <functional>

class Logger
{
public:
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    // SourceFile的作用是提取报错日志所在的文件名
    class SourceFile 
    {
    public : 
        explicit SourceFile(const char* filename)
            : data_(filename) 
        {
            // 查找字符 '/' 的最后一次出现位置
            const char * slash = strrchr(filename , '/') ; 
            if(slash) 
            {
                data_ = slash + 1 ; 
            }
            size_ = static_cast<int>(strlen(data_));
        }

        const char* data_;
        int size_;
    } ; 

    // member function
    Logger(const char* file, int line, LogLevel level);
    Logger(const char* file, int line, LogLevel level, const char* func);
    ~Logger();
 
    // LogStream& stream() { return impl_.stream_; }

    void writeLog(const char *format, ...) ;

    static LogLevel g_logLevel ; 
    static LogLevel logLevel() ;
    static void setLogLevel(LogLevel level);

    // 输出函数和刷新缓冲区函数
    using OutputFunc = std::function<void(const char* msg, int len)>;
    using FlushFunc = std::function<void()>;
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

private : 
     
    void formatTime();
    void finish();

    Timestamp time_ ;
    FixedBuffer<kSmallBuffer> buffer_ ;
    LogLevel level_ ;
    int line_ ;
    SourceFile basename_ ; 
};


/**
 * 当日志等级小于对应等级才会输出
 * 比如设置等级为FATAL，则logLevel等级大于DEBUG和INFO，DEBUG和INFO等级的日志就不会输出
 */
// #define LOG_DEBUG if (logLevel() <= Logger::DEBUG) \
//   Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
// #define LOG_INFO if (logLevel() <= Logger::INFO) \
//   Logger(__FILE__, __LINE__).stream()
// #define LOG_WARN  Logger(__FILE__, __LINE__, Logger::WARN).stream()
// #define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
// #define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()

// 获取errno信息
const char* getErrnoMsg(int savedErrno);

//Logger::LogLevel Logger::g_logLevel = Logger::INFO;

#define LOG_BASE(level, format, ...)\
    do {\
        if (Logger::logLevel() <= level) {\
            Logger(__FILE__, __LINE__, level).writeLog(format, ##__VA_ARGS__);\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(Logger::DEBUG, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(Logger::INFO, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(Logger::WARN, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(Logger::ERROR, format, ##__VA_ARGS__)} while(0);
#define LOG_FATAL(format, ...) do {LOG_BASE(Logger::FATAL, format, ##__VA_ARGS__)} while(0);

#endif // LOGGING_H
 