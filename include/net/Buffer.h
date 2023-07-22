#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <algorithm>

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size

class Buffer
{
public:
    // prependable 初始大小，readIndex 初始位置，开始 readerIndex 和 writerIndex 处于同一位置
    static const size_t kCheapPrepend = 8   ; 
    static const size_t kInitialSize = 1024 ;    

    explicit Buffer(size_t initialSize = kInitialSize)
        :   buffer_(kCheapPrepend + initialSize),
            readerIndex_(kCheapPrepend),
            writerIndex_(kCheapPrepend)
        {}
    
    /**
     * kCheapPrepend | reader | writer |
     * writerIndex_ - readerIndex_
     */
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    /**
     * kCheapPrepend | reader | writer |
     * buffer_.size() - writerIndex_
     */   
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    /**
     * kCheapPrepend | reader | writer |
     * wreaderIndex_
     */    
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    void retrieveUntil(const char *end)
    {
        retrieve(end - peek());
    }
    
    // readerIndex_ , writerIndex_ 复位操作
    void retrieve(size_t len)
    { 
        if (len < readableBytes())
        { 
            readerIndex_ += len;
        } 
        else
        {
            retrieveAll();
        }
    }

    // 全部读完，则直接将可读缓冲区指针移动到写缓冲区指针那
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // DEBUG使用，提取出string类型，但是不会将 reader\writerIndex_ 置位
    std::string GetBufferAllAsString()
    { 
        std::string result(peek(), readableBytes()) ;
        return result;
    }

    // 将onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {    
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    { 
        if(len <= readableBytes()) {
            std::string result(peek(), len);
            retrieve(len); 
            return result;
        }
        return retrieveAllAsString();
    }

    // buffer_.size() - writeIndex_ 保证可写，容量不够则进行扩容
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    void append(const std::string &str)
    {
        append(str.data(), str.size());
    }

    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);
    
private:

    // 获取buffer_起始地址
    char* begin()
    {    
        return &(*buffer_.begin());
    }

    const char* begin() const
    {
        return &(*buffer_.begin());
    }

    void makeSpace(int len)
    {
        /**
         * kCheapPrepend | ... reader | writer |
         * kCheapPrepend |           len         |
         */
        // 整个buffer都不够用
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else // 整个buffer够用，将后面移动到前面继续分配
        {
            size_t readable = readableBytes() ;
            // Copies the range [first,last) into result.
            std::copy(begin() + readerIndex_ , begin() + writerIndex_ , begin() + kCheapPrepend) ;
            readerIndex_ = kCheapPrepend ;
            writerIndex_ = readerIndex_ + readable ;
        }
    }
    std::vector<char> buffer_; // 采取 vector 形式，可以自动分配内存 , 也可以提前预留空间大小
    size_t readerIndex_;
    size_t writerIndex_;
    static const char kCRLF[] ;
};

#endif