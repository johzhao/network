#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <memory>

#include "event_poller.h"
#include "socket_exception.h"
#include "utils/buffer.h"

class TcpClient {
public:
    using Ptr = std::shared_ptr<TcpClient>;

    using OnErrorCB = std::function<void(const SocketException &err)>;

    using OnReceivedCB = std::function<void(const Buffer::Ptr &buf)>;

    using OnSendResultCB = std::function<void(const Buffer::Ptr &buffer, bool send_success)>;

    explicit TcpClient(const EventPoller::Ptr &event_poller);

    ~TcpClient();

public:
    void SetOnErrorCallback(const OnErrorCB &cb);

    void SetOnReceivedCallback(const OnReceivedCB &cb);

    void SetOnSendResultCallback(const OnSendResultCB &cb);

    void Connect(const std::string &url, uint16_t port, const OnErrorCB &cb, float timeout_seconds = 5,
                 const std::string &local_ip = "::", uint16_t local_port = 0);

    ssize_t Send(const char *data, size_t size = 0);

private:
    int CreateSocket(const std::string &local_ip = "::", uint16_t local_port = 0);

    ssize_t HandleReadableEvent();

    void HandleWritableEvent();

    void HandleErrorEvent();

    void CloseSocket();

private:
    EventPoller::Ptr event_poller_;
    OnErrorCB on_error_callback_;
    OnReceivedCB on_received_callback_;
    OnSendResultCB on_send_result_callback_;
    int socket_ = 0;
};

#endif //TCP_CLIENT_H
