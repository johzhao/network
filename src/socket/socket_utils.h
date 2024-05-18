#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <cstdint>
#include <string>

#include "error_code.h"

static constexpr int SOCKET_DEFAULT_BUF_SIZE = 256 * 1024;

class SocketUtils {
public:
    /**
    * 创建tcp客户端套接字并连接服务器
    * @param fd 套接字
    * @param host 服务器ip或域名
    * @param port 服务器端口号
    * @param async 是否异步连接
    * @param local_ip 绑定的本地网卡ip
    * @param local_port 绑定的本地端口号
    * @return ErrorCode
    */
    static ErrorCode connect(int &fd, const char *host, uint16_t port, bool async = true,
                             const char *local_ip = "0.0.0.0", uint16_t local_port = 0);

    /**
     * 创建tcp监听套接字
     * @param fd 套接字
     * @param port 监听的本地端口
     * @param local_ip 绑定的本地网卡ip
     * @param back_log accept列队长度
     * @return ErrorCode
     */
    static ErrorCode listen(int &fd, uint16_t port, const char *local_ip = "0.0.0.0", int back_log = 1024);

    /**
     * 创建udp套接字
     * @param fd 套接字
     * @param port 监听的本地端口
     * @param local_ip 绑定的本地网卡ip
     * @param enable_reuse 是否允许重复bind端口
     * @return ErrorCode
     */
    static ErrorCode bindUdpSock(int &fd, uint16_t port, const char *local_ip = "0.0.0.0", bool enable_reuse = true);

    /**
     * 开启TCP_NODELAY，降低TCP交互延时
     * @param fd socket fd号
     * @param on 是否开启
     * @return 0代表成功，-1为失败
     */
    static int setNoDelay(int fd, bool on = true);

    /**
     * 写socket不触发SIG_PIPE信号(貌似只有mac有效)
     * @param fd socket fd号
     * @return 0代表成功，-1为失败
     */
    static int setNoSigpipe(int fd);

    /**
     * 设置读写socket是否阻塞
     * @param fd socket fd号
     * @param noblock 是否阻塞
     * @return 0代表成功，-1为失败
     */
    static int setNoBlocked(int fd, bool noblock = true);

    /**
     * 设置socket接收缓存，默认貌似8K左右，一般有设置上限
     * 可以通过配置内核配置文件调整
     * @param fd socket fd号
     * @param size 接收缓存大小
     * @return 0代表成功，-1为失败
     */
    static int setRecvBuf(int fd, int size);

    /**
     * 设置socket发送缓存，默认貌似8K左右，一般有设置上限
     * 可以通过配置内核配置文件调整
     * @param fd socket fd号
     * @param size 发送缓存大小
     * @return 0代表成功，-1为失败
     */
    static int setSendBuf(int fd, int size);

    /**
     * 设置后续可绑定复用端口(处于TIME_WAITE状态)
     * @param fd socket fd号
     * @param on 是否开启该特性
     * @return 0代表成功，-1为失败
     */
    static int setReuseable(int fd, bool on = true);

    /**
     * 是否开启FD_CLOEXEC特性(多进程相关)
     * @param fd fd号，不一定是socket
     * @param on 是否开启该特性
     * @return 0代表成功，-1为失败
     */
    static int setCloExec(int fd, bool on = true);

    /**
     * 开启SO_LINGER特性
     * @param sock socket fd号
     * @param second 内核等待关闭socket超时时间，单位秒
     * @return 0代表成功，-1为失败
     */
    static int setCloseWait(int sock, int second = 0);

private:
    SocketUtils() {}
};

#endif //SOCKET_UTILS_H
