#include "socket_utils.h"

#include <asm-generic/ioctls.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "spdlog/spdlog.h"

static ErrorCode bind_sock4(int fd, const char *ifr_ip, uint16_t port);

static int get_uv_error();

static int uv_translate_posix_error(int err);

ErrorCode SocketUtils::bind(int fd, uint16_t port, const char *local_ip) {
    return bind_sock4(fd, local_ip, port);
}

ErrorCode SocketUtils::connect(int fd, const char *host, uint16_t port, bool async) {
    sockaddr_in dest_addr{};
    memset(&dest_addr, 0, sizeof(dest_addr));

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(host);

    if (::connect(fd, (sockaddr *) &dest_addr, sizeof(sockaddr)) == 0) {
        //同步连接成功
        return Success;
    }

    if (async && errno == EINPROGRESS) {
        //异步连接成功
        return Socket_Connect_In_Progress;
    }

    SPDLOG_ERROR("socket connect to {0}:{1} failed with error {2}, description '{3}'",
                 host, port, errno, strerror(errno));

    return Socket_Connect_Failed;
}

ErrorCode SocketUtils::listen(int fd, int back_log) {
    if (::listen(fd, back_log) == -1) {
        SPDLOG_ERROR("socket listen with back log {0} failed with error {1}, description '{2}'",
                     back_log, errno, strerror(errno));

        return Socket_Listen_Failed;
    }

    return Success;
}

int SocketUtils::setNoDelay(int fd, bool on) {
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        SPDLOG_TRACE("setsockopt TCP_NODELAY failed with error {0}, description '{1}'", errno, strerror(errno));
    }

    return ret;
}

int SocketUtils::setNoSigpipe(int /*fd*/) {
    return -1;
}

int SocketUtils::setNoBlocked(int fd, bool noblock) {
    int ul = noblock;
    int ret = ioctl(fd, FIONBIO, &ul);
    if (ret == -1) {
        SPDLOG_TRACE("ioctl FIONBIO failed with error {0}, description '{1}'", errno, strerror(errno));

        return -1;
    }

    return 0;
}

int SocketUtils::setRecvBuf(int fd, int size) {
    if (size <= 0) {
        // use the system default value
        return 0;
    }

    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &size, sizeof(size));
    if (ret == -1) {
        SPDLOG_TRACE("setsockopt SO_RCVBUF failed with error {0}, description '{1}'", errno, strerror(errno));

        return ret;
    }

    return 0;
}

int SocketUtils::setSendBuf(int fd, int size) {
    if (size <= 0) {
        return 0;
    }

    int ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &size, sizeof(size));
    if (ret == -1) {
        SPDLOG_TRACE("setsockopt SO_SNDBUF failed with error {0}, description '{1}'", errno, strerror(errno));

        return ret;
    }

    return 0;
}

int SocketUtils::setReuseable(int fd, bool on) {
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        SPDLOG_TRACE("setsockopt SO_REUSEADDR failed with error {0}, description '{1}'", errno, strerror(errno));

        return ret;
    }

    return 0;
}

int SocketUtils::setCloExec(int fd, bool on) {
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        SPDLOG_TRACE("fcntl F_GETFD failed with error {0}, description '{1}'", errno, strerror(errno));

        return flags;
    }

    if (on) {
        flags |= FD_CLOEXEC;
    } else {
        int cloexec = FD_CLOEXEC;
        flags &= ~cloexec;
    }

    int ret = fcntl(fd, F_SETFD, flags);
    if (ret == -1) {
        SPDLOG_TRACE("fcntl F_SETFD failed with error {0}, description '{1}'", errno, strerror(errno));

        return ret;
    }

    return 0;
}

int SocketUtils::setCloseWait(int fd, int second) {
    linger m_sLinger{};
    //在调用closesocket()时还有数据未发送完，允许等待
    // 若m_sLinger.l_onoff=0;则调用closesocket()后强制关闭
    m_sLinger.l_onoff = (second > 0);
    m_sLinger.l_linger = second; //设置等待时间为x秒

    int ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &m_sLinger, sizeof(linger));
    if (ret == -1) {
        SPDLOG_TRACE("setsockopt SO_LINGER failed with error {0}, description '{1}'", errno, strerror(errno));

        return ret;
    }

    return 0;
}

static ErrorCode bind_sock4(int fd, const char *ifr_ip, uint16_t port) {
    sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (port > 0) {
        addr.sin_port = htons(port);
    }

    if (ifr_ip == nullptr || strcmp(ifr_ip, "0.0.0.0") == 0) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        addr.sin_addr.s_addr = inet_addr(ifr_ip);
    }

    if (::bind(fd, (struct sockaddr *) &addr, sizeof(sockaddr)) == -1) {
        SPDLOG_ERROR("socket bind to {0}:{1} failed with error {2}, description '{3}'",
                     ifr_ip, port, errno, strerror(errno));
        return Socket_Bind_Failed;
    }

    return Success;
}

int get_uv_error() {
    return uv_translate_posix_error(errno);
}

int uv_translate_posix_error(int err) {
    if (err <= 0) {
        return err;
    }

    switch (err) {
        //为了兼容windows/unix平台，信号EINPROGRESS ，EAGAIN，EWOULDBLOCK，ENOBUFS 全部统一成EAGAIN处理
        case ENOBUFS://在mac系统下实测发现会有此信号发生
        case EINPROGRESS:
        case EWOULDBLOCK:
            err = EAGAIN;
            break;
        default:
            break;
    }

    return -err;
}
