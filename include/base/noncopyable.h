#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

// 非常妙的一个操作，禁止派生类的拷贝 和 赋值  构造
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif // NONCOPYABLE_H