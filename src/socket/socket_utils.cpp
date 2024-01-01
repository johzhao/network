#include "socket_utils.h"

#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

#include "loguru/loguru.hpp"

int SocketUtils::SetNoDelay(int fd, bool on) {
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret) {
        LOG_F(ERROR, "setsockopt TCP_NODELAY failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    return 0;
}

int SocketUtils::SetNoBlocked(int fd, bool noblock) {
    int ul = noblock;
    int ret = ioctl(fd, FIONBIO, &ul); //设置为非阻塞模式
    if (ret) {
        LOG_F(ERROR, "ioctl FIONBIO failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    return 0;
}

int SocketUtils::SetRecvBuf(int fd, int size) {
    if (size <= 0) {
        return 0;
    }

    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &size, sizeof(size));
    if (ret) {
        LOG_F(ERROR, "setsockopt SO_RCVBUF failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }
    return 0;
}

int SocketUtils::SetSendBuf(int fd, int size) {
    if (size <= 0) {
        return 0;
    }

    int ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &size, sizeof(size));
    if (ret) {
        LOG_F(ERROR, "setsockopt SO_SNDBUF failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    return 0;
}

int SocketUtils::SetReuseable(int fd, bool on, bool reuse_port) {
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        LOG_F(ERROR, "setsockopt SO_REUSEADDR failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

#if defined(SO_REUSEPORT)
    if (reuse_port) {
        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
        if (ret == -1) {
            LOG_F(ERROR, "setsockopt SO_REUSEPORT failed with error %d, message '%s'", errno, strerror(errno));
            return ret;
        }
    }
#endif

    return 0;
}

int SocketUtils::SetBroadcast(int fd, bool on) {
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        LOG_F(ERROR, "setsockopt SO_BROADCAST failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    return 0;
}

int SocketUtils::SetKeepAlive(int fd, bool on, int interval, int idle, int times) {
    // Enable/disable the keep-alive option
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret) {
        LOG_F(ERROR, "setsockopt SO_KEEPALIVE failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    // Set the keep-alive parameters
    if (on && interval > 0) {
        ret = setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (char *) &idle, static_cast<socklen_t>(sizeof(idle)));
        if (ret) {
            LOG_F(ERROR, "setsockopt TCP_KEEPIDLE failed with error %d, message '%s'", errno, strerror(errno));
            return ret;
        }

        ret = setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (char *) &interval, static_cast<socklen_t>(sizeof(interval)));
        if (ret) {
            LOG_F(ERROR, "setsockopt TCP_KEEPINTVL failed with error %d, message '%s'", errno, strerror(errno));
            return ret;
        }

        ret = setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (char *) &times, static_cast<socklen_t>(sizeof(times)));
        if (ret) {
            LOG_F(ERROR, "setsockopt TCP_KEEPCNT failed with error %d, message '%s'", errno, strerror(errno));
            return ret;
        }
    }

    return 0;
}

int SocketUtils::SetCloExec(int fd, bool on) {
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        LOG_F(ERROR, "fcntl F_GETFD failed with error %d, message '%s'", errno, strerror(errno));
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
        LOG_F(ERROR, "fcntl F_SETFD failed with error %d, message '%s'", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int SocketUtils::SetCloseWait(int fd, int second) {
    //在调用closesocket()时还有数据未发送完，允许等待
    // 若m_sLinger.l_onoff=0;则调用closesocket()后强制关闭
    linger m_sLinger;
    m_sLinger.l_onoff = (second > 0);
    m_sLinger.l_linger = second; //设置等待时间为x秒
    int ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &m_sLinger, sizeof(linger));
    if (ret) {
        LOG_F(ERROR, "setsockopt SO_LINGER failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    return 0;
}

SocketException SocketUtils::GetSockErr(int sock, bool try_errno) {
    int error = 0;
    int len = sizeof(int);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &error, (socklen_t *) &len);
    if (error == 0) {
        if (try_errno) {
            error = errno;
        }
    }

    return ToSockException(error);
}

SocketException SocketUtils::ToSockException(int error) {
    switch (error) {
        case 0:
        case EAGAIN:
            return SocketException(Err_success, "success");
        case ECONNREFUSED:
            return SocketException(Err_refused, strerror(error), error);
        case ETIMEDOUT:
            return SocketException(Err_timeout, strerror(error), error);
        case ECONNRESET:
            return SocketException(Err_reset, strerror(error), error);
        default:
            return SocketException(Err_other, strerror(error), error);
    }
}
