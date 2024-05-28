#ifndef SOCKET_H
#define SOCKET_H

#include <functional>
#include <memory>
#include <list>
#include <unistd.h>

#include "poll_thread.h"
#include "utils/buffer.h"
#include "utils/buffer_sock.h"

enum class SocketType {
    Invalid = 0,
    TcpServer,
    TcpClient,
    Udp,
};

class Socket : public std::enable_shared_from_this<Socket> {
public:
    explicit Socket(std::string id, std::shared_ptr<PollThread> &poll_thread);

    ~Socket();

    using OnReadCallback = std::function<void(Buffer::Ptr &buf, sockaddr *addr, int addr_len)>;
    using OnErrCallback = std::function<void(ErrorCode error_code)>;
    using OnAcceptCallback = std::function<void(std::shared_ptr<Socket> &sock, sockaddr *addr, int addr_len)>;
    using OnBeforeCreateCallback = std::function<std::shared_ptr<Socket>()>;
    //using OnFlushCallback = std::function<bool()>;
    using OnSentResultCallback = std::function<void(Buffer::Ptr &buffer, bool send_success)>;
    using OnClosedCallback = std::function<void()>;

public:
    const std::string &GetId() const;

    int GetRawSocket() const;

    ErrorCode Initialize(SocketType type, bool async = true);

    ErrorCode Bind(uint16_t port, const std::string &local_ip = "0.0.0.0");

    /**
     * 创建tcp客户端并异步连接服务器
     * @param host 目标服务器ip
     * @param port 目标服务器端口
     * @param con_cb 结果回调
     * @param timeout_sec 超时时间
     */
    void Connect(const std::string &host, uint16_t port, const OnErrCallback &error_callback,
                 float timeout_sec = 5);

    /**
     * 创建tcp监听服务器
     * @param port 监听端口，0则随机
     * @param local_ip 监听的网卡ip
     * @param backlog tcp最大积压数
     * @return 是否成功
     */
    ErrorCode Listen(int backlog = 1024);

    /**
     * 包装外部fd，本对象负责close fd
     * 内部会设置fd为NoBlocked,NoSigpipe,CloExec
     * 其他设置需要自行使用SockUtil进行设置
     */
    //bool fromSock(int fd, SockNum::SockType type);

    /**
     * 从另外一个Socket克隆
     * 目的是一个socket可以被多个poller对象监听，提高性能或实现Socket归属线程的迁移
     * @param other 原始的socket对象
     * @return 是否成功
     */
    //bool cloneSocket(const Socket &other);

    ////////////设置事件回调////////////

    /**
     * 设置数据接收回调,tcp或udp客户端有效
     * @param cb 回调对象
     */
    void SetOnReadCallback(OnReadCallback callback);

    /**
     * 设置异常事件(包括eof等)回调
     * @param cb 回调对象
     */
    void SetOnErrorCallback(OnErrCallback callback);

    /**
     * 设置tcp监听接收到连接回调
     * @param cb 回调对象
     */
    void SetOnAcceptCallback(OnAcceptCallback callback);

    /**
     * 设置socket写缓存清空事件回调
     * 通过该回调可以实现发送流控
     * @param cb 回调对象
     */
    //void setOnFlush(onFlush cb);

    /**
     * 设置accept时，socket构造事件回调
     * @param cb 回调
     */
    void SetOnBeforeCreateCallback(OnBeforeCreateCallback callback);

    /**
     * 设置发送buffer结果回调
     * @param cb 回调
     */
    void SetOnSentResultCallback(OnSentResultCallback callback);

    void SetOnClosedCallback(OnClosedCallback callback);

    ////////////发送数据相关接口////////////

    /**
     * 发送数据指针
     * @param buf 数据指针
     * @param size 数据长度
     * @param addr 目标地址
     * @param addr_len 目标地址长度
     * @param try_flush 是否尝试写socket
     * @return -1代表失败(socket无效)，0代表数据长度为0，否则返回数据长度
     */
    //ssize_t Send(const char *buf, size_t size = 0, struct sockaddr *addr = nullptr, socklen_t addr_len = 0, bool try_flush = true);

    /**
     * 发送string
     */
    //ssize_t Send(std::string buf, struct sockaddr *addr = nullptr, socklen_t addr_len = 0, bool try_flush = true);

    /**
     * 发送Buffer对象，Socket对象发送数据的统一出口
     * socket对象发送数据的统一出口
     */
    ssize_t Send(Buffer::Ptr &buf, bool try_flush = true);

    ssize_t SendTo(Buffer::Ptr &buf, const char *host = nullptr, uint16_t port = 0, bool try_flush = true);

    ssize_t Send(Buffer::Ptr &buf, sockaddr *addr, socklen_t addr_len, bool try_flush);

    /**
     * 尝试将所有数据写socket
     * @return -1代表失败(socket无效或者发送超时)，0代表成功?
     */
    //int flushAll();

    /**
     * 关闭socket且触发onErr回调，onErr回调将在poller线程中进行
     * @param err 错误原因
     * @return 是否成功触发onErr回调
     */
    //bool emitErr(const SockException &err) noexcept;

    /**
     * 关闭或开启数据接收
     * @param enabled 是否开启
     */
    //void enableRecv(bool enabled);

    /**
     * tcp客户端是否处于连接状态
     * 支持Sock_TCP类型socket
     */
    //bool alive() const;

    /**
     * 返回socket类型
     */
    //SockNum::SockType sockType() const;

    /**
     * 设置发送超时主动断开时间;默认10秒
     * @param seconds 发送超时数据，单位秒
     */
    void SetSendTimeOutSecond(uint32_t seconds);

    /**
     * 套接字是否忙，如果套接字写缓存已满则返回true
     * @return 套接字是否忙
     */
    //bool isSocketBusy() const;

    /**
     * 获取poller线程对象
     * @return poller线程对象
     */
    //const EventPoller::Ptr &getPoller() const;

    /**
     * 绑定udp 目标地址，后续发送时就不用再单独指定了
     * @param dst_addr 目标地址
     * @param addr_len 目标地址长度
     * @param soft_bind 是否软绑定，软绑定时不调用udp connect接口，只保存目标地址信息，发送时再传递到sendto函数
     * @return 是否成功
     */
    //bool BindPeerAddr(const struct sockaddr *dst_addr, socklen_t addr_len = 0, bool soft_bind = false);

    /**
     * 设置发送flags
     * @param flags 发送的flag
     */
    //void SetSendFlags(int flags = SOCKET_DEFAULE_FLAGS);

    /**
     * 关闭套接字
     */
    void Close();

    /**
     * 获取发送缓存包个数(不是字节数)
     */
    //size_t getSendBufferCount();

    /**
     * 获取上次socket发送缓存清空至今的毫秒数,单位毫秒
     */
    //uint64_t elapsedTimeAfterFlushed();

    /**
     * 获取接收速率，单位bytes/s
     */
    //int getRecvSpeed();

    /**
     * 获取发送速率，单位bytes/s
     */
    //int getSendSpeed();

    ////////////SockInfo ////////////

    std::string GetLocalIp();

    uint16_t GetLocalPort();

    std::string GetPeerIp();

    uint16_t GetPeerPort();

private:
    void RegisterEvent();

    void StartWritableEvent();

    void StopWritableEvent();

    void UnRegisterEvent();

    void OnPollEvent(int event);

    void OnAcceptEvent();

    void OnReadableEvent();

    void OnWritableEvent();

    void OnErrorEvent();

    void Flush(bool by_poll_thread);

private:
    std::string id_;
    std::shared_ptr<PollThread> poll_thread_;
    SocketType socket_type_ = SocketType::Invalid;
    int socket_fd_ = 0;
    bool is_async_ = true;
    OnErrCallback connect_callback_;
    OnReadCallback read_callback_;
    OnErrCallback error_callback_;
    OnAcceptCallback accept_callback_;
    OnBeforeCreateCallback before_create_callback_;
    OnSentResultCallback sent_result_callback_;
    OnClosedCallback closed_callback_;
    std::mutex send_queue_mutex_;
    std::list<std::shared_ptr<BufferSock>> send_queue_;
    std::mutex sending_buffer_mutex_;
    std::shared_ptr<BufferSock> sending_buffer_ = nullptr;
    std::atomic<bool> available_send_ = {false};
    int send_flags_ = 0;
    std::atomic<bool> connecting_{false};
    int next_accepted_id_ = 0;
};

#endif //SOCKET_H
