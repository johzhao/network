#include "tcp_client.h"

#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#include "loguru/loguru.hpp"
#include "socket_utils.h"

TcpClient::TcpClient(const EventPoller::Ptr &event_poller) : event_poller_(event_poller) {
    SetOnErrorCallback(nullptr);
    SetOnReceivedCallback(nullptr);
    SetOnSendResultCallback(nullptr);
}

TcpClient::~TcpClient() = default;

void TcpClient::SetOnErrorCallback(const OnErrorCB &cb) {
    if (cb) {
        on_error_callback_ = cb;
    } else {
        on_error_callback_ = [](const SocketException &) {};
    }
}

void TcpClient::SetOnReceivedCallback(const OnReceivedCB &cb) {
    if (cb) {
        on_received_callback_ = cb;
    } else {
        on_received_callback_ = [](const Buffer::Ptr &) {};
    }
}

void TcpClient::SetOnSendResultCallback(const OnSendResultCB &cb) {
    if (cb) {
        on_send_result_callback_ = cb;
    } else {
        on_send_result_callback_ = [](const Buffer::Ptr &, bool) {};
    }
}

void TcpClient::Connect(const std::string &url, uint16_t port, const TcpClient::OnErrorCB &cb, float timeout_seconds,
                        const std::string &local_ip, uint16_t local_port) {
    int ret = CreateSocket(local_ip, local_port);
    if (ret) {
        cb(SocketException(Err_other, strerror(ret), ret));
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(url.c_str());
    ret = connect(socket_, (sockaddr *) &addr, sizeof(addr));
    if (ret) {
        LOG_F(ERROR, "connect failed with error %d, message '%s'", errno, strerror(errno));
        cb(SocketException(Err_other, strerror(ret), ret));

        return;
    }

    cb(SocketException());
}

ssize_t TcpClient::Send(const char *data, size_t size) {
    if (size == 0) {
        size = strlen(data);
        if (!size) {
            return size;
        }
    }

    // TODO

    return size;
}

int TcpClient::CreateSocket(const std::string &local_ip, uint16_t local_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_F(ERROR, "create socket failed with error: %d, message: '%s'", errno, strerror(errno));
        return errno;
    }

    SocketUtils::SetReuseable(sockfd);
    SocketUtils::SetNoBlocked(sockfd);
    SocketUtils::SetNoDelay(sockfd);
    SocketUtils::SetSendBuf(sockfd);
    SocketUtils::SetRecvBuf(sockfd);
    SocketUtils::SetCloseWait(sockfd);
    SocketUtils::SetCloExec(sockfd);

    int ret;
    if (local_port > 0) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(local_port);
        addr.sin_addr.s_addr = INADDR_ANY;

        ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
        if (ret) {
            LOG_F(ERROR, "bind socket failed with error: %d, message: '%s'", errno, strerror(errno));
            close(sockfd);

            return errno;
        }
    }

    event_poller_->AddEvent(sockfd, Event_Read | Event_Write | Event_Error | Event_LT, [this](int events) {
        if (events & Event_Read) {
            this->HandleReadableEvent();
        } else if (events & Event_Write) {
            this->HandleWritableEvent();
        } else if (events & Event_Error) {
            this->HandleErrorEvent();
        }
    });

    socket_ = sockfd;

    return 0;
}

ssize_t TcpClient::HandleReadableEvent() {
    auto buffer = event_poller_->GetSharedBuffer();

    auto data = buffer->GetData();
    auto capacity = buffer->GetCapacity();

    ssize_t ret = 0;
    while (true) {
        ssize_t read_count = recv(socket_, data, capacity, 0);
        if (read_count < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_F(ERROR, "tcp client receive failed with error %d, message '%s'", errno, strerror(errno));
            }

            break;
        }

        if (read_count == 0) {
            break;
        }

        ret += read_count;
        buffer->SetSize(read_count);

        try {
            on_received_callback_(buffer);
        } catch (std::exception &ex) {
            LOG_F(ERROR, "tcp client on received callback trigger exception '%s'", ex.what());
        }
    }

    return ret;
}

void TcpClient::HandleWritableEvent() {
    // TODO
}

void TcpClient::HandleErrorEvent() {
    int error = SocketUtils::GetSockErr(socket_);
    auto exception = SocketUtils::ToSockException(error);

    on_error_callback_(exception);
}

void TcpClient::CloseSocket() {
    // TODO
}
