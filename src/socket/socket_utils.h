#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include "socket_exception.h"

#define SOCKET_DEFAULT_BUF_SIZE (256 * 1024)
#define TCP_KEEPALIVE_INTERVAL 30
#define TCP_KEEPALIVE_PROBE_TIMES 9
#define TCP_KEEPALIVE_TIME 120

class SocketUtils {
public:

    /**
     * 开启TCP_NODELAY，降低TCP交互延时
     * @param fd socket fd号
     * @param on 是否开启
     * @return 0代表成功，-1为失败
     */
    static int SetNoDelay(int fd, bool on = true);

    /**
     * 设置读写socket是否阻塞
     * @param fd socket fd号
     * @param noblock 是否阻塞
     * @return 0代表成功，-1为失败
     */
    static int SetNoBlocked(int fd, bool noblock = true);

    /**
     * 设置socket接收缓存，默认貌似8K左右，一般有设置上限
     * 可以通过配置内核配置文件调整
     * @param fd socket fd号
     * @param size 接收缓存大小
     * @return 0代表成功，-1为失败
     */
    static int SetRecvBuf(int fd, int size = SOCKET_DEFAULT_BUF_SIZE);

    /**
     * 设置socket接收缓存，默认貌似8K左右，一般有设置上限
     * 可以通过配置内核配置文件调整
     * @param fd socket fd号
     * @param size 接收缓存大小
     * @return 0代表成功，-1为失败
     */
    static int SetSendBuf(int fd, int size = SOCKET_DEFAULT_BUF_SIZE);

    /**
     * 设置后续可绑定复用端口(处于TIME_WAITE状态)
     * @param fd socket fd号
     * @param on 是否开启该特性
     * @return 0代表成功，-1为失败
     */
    static int SetReuseable(int fd, bool on = true, bool reuse_port = true);

    /**
     * 运行发送或接收udp广播信息
     * @param fd socket fd号
     * @param on 是否开启该特性
     * @return 0代表成功，-1为失败
     */
    static int SetBroadcast(int fd, bool on = true);

    /**
     * 是否开启TCP KeepAlive特性
     * @param fd socket fd号
     * @param on 是否开启该特性
     * @param idle keepalive空闲时间
     * @param interval keepalive探测时间间隔
     * @param times keepalive探测次数
     * @return 0代表成功，-1为失败
     */
    static int
    SetKeepAlive(int fd, bool on = true, int interval = TCP_KEEPALIVE_INTERVAL, int idle = TCP_KEEPALIVE_TIME,
                 int times = TCP_KEEPALIVE_PROBE_TIMES);

    /**
     * 是否开启FD_CLOEXEC特性(多进程相关)
     * @param fd fd号，不一定是socket
     * @param on 是否开启该特性
     * @return 0代表成功，-1为失败
     */
    static int SetCloExec(int fd, bool on = true);

    /**
     * 开启SO_LINGER特性
     * @param fd socket fd号
     * @param second 内核等待关闭socket超时时间，单位秒
     * @return 0代表成功，-1为失败
     */
    static int SetCloseWait(int sock, int second = 0);

    static SocketException GetSockErr(int sock, bool try_errno = true);

    static SocketException ToSockException(int error);
};

#endif //SOCKET_UTILS_H
