## muduo 网络库的设计记录

EventLoop ：
1. 是 Epoller & Channel 之间操作的桥梁
2. 可以被 eventFd 写入 wakeup 执行对应的回调函数
3. 通过线程 id 区分是否执行当前线程中队列中的回调函数

EventLoopThread: 
1. 比较巧妙的是 subLoop 是局部变量，在 Thread 中启用死循环是创建局部变量 Loop 循环监听请求，然后把 Loop 返回回去，这样之后就不用考虑析构的问题了
2. Loop 中的就是 Epoll_wait 循环监听就绪事件