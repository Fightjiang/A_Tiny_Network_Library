#ifndef LOG_FILE_H
#define LOG_FILE_H

 
#include <memory>
#include <stdio.h>
#include "./log/Logging.h"

class LogFile
{
public:
    LogFile(off_t rollSize = 5 * 1024 * 1024,
            int flushInterval = 3,
            int checkEveryN = 0);
    ~LogFile();

    void append(const char* data, int len);
    void flush();
    bool rollFile(); // 滚动日志

private:
    class FileUtil
    {
    public:
        explicit FileUtil(std::string& fileName); 
        ~FileUtil();

        void append(const char* data, size_t len);

        void flush();

        off_t writtenBytes() const { return writtenBytes_; }

    private:    
        size_t write(const char* data, size_t len);

        FILE* fp_;
        char buffer_[64 * 1024]; // fp_的缓冲区
        off_t writtenBytes_; // off_t用于指示文件的偏移量
    };

    static std::string getLogFileName(time_t* now); 
 
    const off_t rollSize_;
    const int flushInterval_;
    const int checkEveryN_;

    int count_;
 
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<FileUtil> file_;

    const static int kRollPerSeconds_ = 60*60*24;
};

#endif // LOG_FILE_H