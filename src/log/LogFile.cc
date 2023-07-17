#include "./log/LogFile.h"

LogFile::LogFile(off_t rollSize,
        int flushInterval,
        int checkEveryN)
    : rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0), 
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0)
{
    rollFile();
}

LogFile::~LogFile() = default;

// 后台日志是单例模式，这里没有加锁，保证只有一个后台线程在写入日志到磁盘中 
void LogFile::append(const char* data, int len)
{
    file_->append(data, len);
    // 文件已经写入了 writtenBytes 大小，判断是否需要回滚
    if (file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }
    else
    {
        ++count_; // 默认是 0 , 调用 checkEveryN_ 次之后，判断是否需要创建新的 file ，或者刷新到磁盘里
        if (count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
            // 如果超过一天，就会创建一个新的日志文件
            if (thisPeriod != startOfPeriod_)
            {
                rollFile();
            } // flush 文件操作也不会频繁执行，默认的时间间隔是 3 秒
            else if (now - lastFlush_ > flushInterval_)
            {
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
    file_->flush();
}

void LogFile::flush()
{
    file_->flush();
}


// 滚动日志
// basename + time + ".log"
bool LogFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(&now);
    // 计算现在是第几天 now/kRollPerSeconds求出现在是第几天，再乘以秒数相当于是当前天数0点对应的秒数
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    // 为了避免频繁创建新文件，rollFile会确保上次滚动时间到现在如果不到1秒，就不会滚动。
    if (now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        // 让file_指向一个名为filename的文件，相当于新建了一个文件
        file_.reset(new FileUtil(filename));
        return true;
    }
    return false;
}

// 这个需要调整下
std::string LogFile::getLogFileName(time_t* now)
{
    std::string filename;
    filename.reserve(64); 

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now, &tm);
    // 写入时间
    strftime(timebuf, sizeof(timebuf), "%Y_%m_%d:%H_%M_%S", &tm);
    filename += timebuf;

    filename += ".log";

    return filename;
}

LogFile::FileUtil::FileUtil(std::string& fileName)
    : fp_(::fopen(fileName.c_str(), "ae")),
      writtenBytes_(0)
{
    // 将fd_缓冲区设置为本地的buffer_
    ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

LogFile::FileUtil::~FileUtil()
{
    ::fclose(fp_);
}

void LogFile::FileUtil::append(const char* data, size_t len)
{
    // 记录已经写入的数据大小
    size_t written = 0;

    while (written != len)
    {
        // 还需写入的数据大小
        size_t remain = len - written;
        size_t n = write(data + written, remain);
        if (n != remain)
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
            }
        }
        // 更新写入的数据大小
        written += n;
    }
    // 记录目前为止写入的数据大小，超过限制会滚动日志
    writtenBytes_ += written;
}

void LogFile::FileUtil::flush()
{
    ::fflush(fp_);
}

size_t LogFile::FileUtil::write(const char* data, size_t len)
{
    /**
     * 它的作用是在多线程或多进程环境中提供对文件流的无锁写入操作，以提高性能和避免竞争条件。
     * size_t fwrite_unlocked(const void* buffer, size_t size, size_t count, FILE* stream);
     * -- buffer:指向数据块的指针
     * -- size:每个数据的大小，单位为Byte(例如：sizeof(int)就是4)
     * -- count:数据个数
     * -- stream:文件指针
     */
    int size = ::fwrite_unlocked(data, 1, len, fp_);
    return size ;
}