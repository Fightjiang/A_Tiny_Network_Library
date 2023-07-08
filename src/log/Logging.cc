#include "base/CurrentThread.h"
#include "log/Logging.h"

namespace ThreadInfo
{
    __thread char t_errnobuf[512];
    __thread char t_time[64];
    __thread time_t t_lastSecond;
};

const char* getErrnoMsg(int savedErrno)
{
    return strerror_r(savedErrno, ThreadInfo::t_errnobuf, sizeof(ThreadInfo::t_errnobuf));
}

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

static void defaultOutput(const char* data, int len)
{
    ::fwrite(data, len, sizeof(char), stdout);
}

static void defaultFlush()
{
    ::fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;
Logger::LogLevel g_logLevel = Logger::INFO ; 

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(Logger::OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(Logger::FlushFunc flush)
{
    g_flush = flush;
}

Logger::LogLevel Logger::logLevel() 
{
    return g_logLevel; 
} 

// Timestamp::toString方法的思路，只不过这里需要输出到流
void Logger::formatTime()
{
    Timestamp now = Timestamp::now();
    time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);

    struct tm *tm_time = localtime(&seconds);
    // 写入此线程存储的时间buf中
    snprintf(ThreadInfo::t_time, sizeof(ThreadInfo::t_time), "%4d/%02d/%02d %02d:%02d:%02d ",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);
   
   // 更新最后一次时间调用
    ThreadInfo::t_lastSecond = seconds;

    buffer_.append(ThreadInfo::t_time) ; 

    // // muduo 使用 Fmt 格式化整数，这里我们直接写入 buf
    // char buf[32] = {0};
    // snprintf(buf, sizeof(buf), ":%06d ", microseconds);

    // buffer_.append(buf , 7) ; // 注意多了一个空格 
}

void Logger::finish()
{
    buffer_.append(" - ") ; 
    buffer_.append(basename_.data_, basename_.size_) ; 
    buffer_.append(" : " + std::to_string(line_) + "\n") ;  
}

// level默认为INFO等级
Logger::Logger(const char* file, int line , Logger::LogLevel level)  
    : time_(Timestamp::now()),
      buffer_(),
      level_(level),
      line_(line),
      basename_(file)
{
    // 固定的时间格式，输出流 -> time
    formatTime();

    // 写入线程 id 
    buffer_.append("tid :" + std::to_string(CurrentThread::tid()) + " ");
     
    // 写入日志等级 
    buffer_.append(LogLevelName[level] , ::strlen(LogLevelName[level])) ;
}
 
// 可以打印调用函数
Logger::Logger(const char* file, int line, Logger::LogLevel level, const char* func)
    : time_(Timestamp::now()), 
      buffer_(),
      level_(level),
      line_(line),
      basename_(file)
{
    // 固定的时间格式，输出流 -> time
    formatTime();

    // 写入日志等级 
    buffer_.append(LogLevelName[level] , ::strlen(LogLevelName[level])) ;
    
    // 写入调用的函数信息
    buffer_.append(func) ;  
}

Logger::~Logger()
{
    finish(); 

    // 输出(默认向终端输出)
    g_output(buffer_.data(), buffer_.length());

    // FATAL情况终止程序
    if (level_ == FATAL)
    {
        g_flush();
        ::abort();
    }
}

void Logger::writeLog(const char *format, ...)
{
    // 写日志内容
    char tmpBuf[286] = {0} ;
    // ... 使用的可变参数列表
    va_list vaList ; 
    // 根据用户传入的参数，向缓冲区添加数据 
    va_start(vaList, format);
    // m 是 vaList 字符串的长度，并不是写入到 buff 中的长度
    int m = vsnprintf(tmpBuf , 286 , format, vaList);
    va_end(vaList); 

    buffer_.append(tmpBuf , m) ; 
}
