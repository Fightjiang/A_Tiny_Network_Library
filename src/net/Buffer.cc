#include <sys/uio.h>
#include <unistd.h>
#include "./net/Buffer.h"
#include "./log/Logging.h"


const char Buffer::kCRLF[] = "\r\n" ;



// 从 fd 上读取数据, Epoller 工作在 LT 模式
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    /**
    * @description: 从socket读到缓冲区的方法是使用readv先读至buffer_，
    * Buffer_空间如果不够会读入到栈上65536个字节大小的空间，然后以append的
    * 方式追加入buffer_。既考虑了避免系统调用带来开销，又不影响数据的接收。
    **/

    /*
    struct iovec {
        ptr_t iov_base; // iov_base指向的缓冲区存放的是readv所接收的数据或是writev将要发送的数据
        size_t iov_len; // iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度
    };
    */
    struct iovec vec[2]; // 使用iovec分配两个连续的缓冲区

    // 第一块缓冲区，指向可写空间
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_ ;
    vec[0].iov_len = writable ;
    // 第二块缓冲区，栈额外空间 65536/1024 = 64KB，用于从套接字往出读时，当buffer_暂时不够用时暂存数据，待buffer_重新分配足够空间后，在把数据交换给buffer_。
    char extrabuf[65536] = {0} ;  
    vec[1].iov_base = extrabuf ;
    vec[1].iov_len = sizeof(extrabuf) ;

    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    // 这里之所以说最多128k-1字节，是因为若writable为64k-1，那么需要两个缓冲区 第一个64k-1 第二个64k 所以做多128k-1
    // 如果第一个缓冲区 >=64k 那就只采用一个缓冲区 而不使用栈空间extrabuf[65536]的内容
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else // extrabuf里面也写入了n-writable长度的数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); // 对buffer_扩容 并将extrabuf存储的另一部分数据追加至buffer_
    }
    return n;
}

//向 fd 中写入数据 , 返回实际写入的数据
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}