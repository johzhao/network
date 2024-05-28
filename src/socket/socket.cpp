#include <netinet/in.h>
#include <arpa/inet.h>
#include "socket.h"

#include "spdlog/spdlog.h"
#include "utils/copy_buffer.h"
#include "socket_utils.h"

Socket::Socket(std::string id, std::shared_ptr<PollThread> &poll_thread)
        : id_(std::move(id)), poll_thread_(poll_thread) {
    SPDLOG_DEBUG("create socket {0}", id_);

    SetOnReadCallback(nullptr);
    SetOnErrorCallback(nullptr);
    SetOnAcceptCallback(nullptr);
    SetOnBeforeCreateCallback(nullptr);
    SetOnSentResultCallback(nullptr);
    SetOnClosedCallback(nullptr);
}

Socket::~Socket() {
    Close();

    SPDLOG_DEBUG("socket {0} was de-constructed", id_);
}

const std::string &Socket::GetId() const {
    return id_;
}

int Socket::GetRawSocket() const {
    return socket_fd_;
}

ErrorCode Socket::Initialize(SocketType type, bool async) {
    switch (type) {
        case SocketType::TcpServer: {
            socket_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (socket_fd_ < 0) {
                SPDLOG_ERROR("socket {} construct tcp socket failed with error {}, description '{}'",
                             id_, errno, strerror(errno));

                return Socket_Create_Failed;
            }

            SocketUtils::setReuseable(socket_fd_);
            SocketUtils::setNoBlocked(socket_fd_, async);
            SocketUtils::setCloExec(socket_fd_);
            break;
        }
        case SocketType::TcpClient: {
            socket_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (socket_fd_ < 0) {
                SPDLOG_ERROR("socket {} construct tcp socket failed with error {}, description '{}'",
                             id_, errno, strerror(errno));

                return Socket_Create_Failed;
            }

            SocketUtils::setReuseable(socket_fd_);
            SocketUtils::setNoSigpipe(socket_fd_);
            SocketUtils::setNoBlocked(socket_fd_, async);
            SocketUtils::setNoDelay(socket_fd_);
            SocketUtils::setSendBuf(socket_fd_, SOCKET_DEFAULT_BUF_SIZE);
            SocketUtils::setRecvBuf(socket_fd_, SOCKET_DEFAULT_BUF_SIZE);
            SocketUtils::setCloseWait(socket_fd_);
            SocketUtils::setCloExec(socket_fd_);
            break;
        }
        case SocketType::Udp: {
            socket_fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (socket_fd_ < 0) {
                SPDLOG_ERROR("socket {} construct udp socket failed with error {}, description '{}'",
                             id_, errno, strerror(errno));

                return Socket_Create_Failed;
            }

            SocketUtils::setReuseable(socket_fd_);
            SocketUtils::setNoSigpipe(socket_fd_);
            SocketUtils::setNoBlocked(socket_fd_, async);
            SocketUtils::setSendBuf(socket_fd_, SOCKET_DEFAULT_BUF_SIZE);
            SocketUtils::setRecvBuf(socket_fd_, SOCKET_DEFAULT_BUF_SIZE);
            SocketUtils::setCloseWait(socket_fd_);
            SocketUtils::setCloExec(socket_fd_);
            break;
        }
        default: {
            SPDLOG_ERROR("socket {} create with invalid socket type {}", id_, int(type));
            return Socket_Create_Failed;
        }
    }

    socket_type_ = type;
    is_async_ = async;

    return Success;
}

ErrorCode Socket::Bind(uint16_t port, const std::string &local_ip) {
    auto ret = SocketUtils::bind(socket_fd_, port, local_ip.c_str());
    if (ret != Success) {
        SPDLOG_ERROR("socket {} bind to {}:{} failed with error 0x{:08X}",
                     id_, local_ip, port, int(ret));
        return ret;
    }

    return Success;
}

void Socket::Connect(const std::string &host, uint16_t port, const OnErrCallback &error_callback,
                     float timeout_sec) {
    SPDLOG_DEBUG("socket {0} connect to {1}:{2}", id_, host, port);
    auto error_code = SocketUtils::connect(socket_fd_, host.c_str(), port, is_async_);
    if (error_code != Success) {
        if (error_code == Socket_Connect_In_Progress) {
            SPDLOG_DEBUG("socket {0} was connecting...", id_);
            connecting_ = true;
            connect_callback_ = error_callback;
        } else {
            SPDLOG_ERROR("socket {0} connect to {1}:{2} failed with code {3}", id_, host, port, int(error_code));
            error_callback(error_code);

            return;
        }
        RegisterEvent();
    } else {
        RegisterEvent();

        error_callback(Success);
    }
}

ErrorCode Socket::Listen(int backlog) {
    SPDLOG_DEBUG("socket {0} start listen", id_);
    switch (socket_type_) {
        case SocketType::TcpServer: {
            auto error_code = SocketUtils::listen(socket_fd_, backlog);
            if (error_code != Success) {
                SPDLOG_ERROR("socket {} listen failed with error 0x{:08X}", id_, int(error_code));

                return error_code;
            }

            RegisterEvent();
            break;
        }
        case SocketType::Udp: {
            RegisterEvent();
            break;
        }
        default: {
            break;
        }
    }

    return Success;
}

void Socket::SetOnReadCallback(OnReadCallback callback) {
    if (callback == nullptr) {
        read_callback_ = [](Buffer::Ptr &, sockaddr *, int) {};
    } else {
        read_callback_ = std::move(callback);
    }
}

void Socket::SetOnErrorCallback(OnErrCallback callback) {
    if (callback == nullptr) {
        error_callback_ = [](ErrorCode) {};
    } else {
        error_callback_ = std::move(callback);
    }
}

void Socket::SetOnAcceptCallback(OnAcceptCallback callback) {
    if (callback == nullptr) {
        accept_callback_ = [](std::shared_ptr<Socket> &, sockaddr *, int) {};
    } else {
        accept_callback_ = std::move(callback);
    }
}

void Socket::SetOnBeforeCreateCallback(OnBeforeCreateCallback callback) {
    if (callback == nullptr) {
        before_create_callback_ = [this]() {
            auto client_id = ++next_accepted_id_;
            return std::make_shared<Socket>(fmt::format("{}-{}", id_, client_id), poll_thread_);
        };
    } else {
        before_create_callback_ = std::move(callback);
    }
}

void Socket::SetOnSentResultCallback(OnSentResultCallback callback) {
    if (callback == nullptr) {
        sent_result_callback_ = [](Buffer::Ptr &, bool) {};
    } else {
        sent_result_callback_ = std::move(callback);
    }
}

void Socket::SetOnClosedCallback(OnClosedCallback callback) {
    if (callback == nullptr) {
        closed_callback_ = []() {};
    } else {
        closed_callback_ = std::move(callback);
    }
}

ssize_t Socket::Send(Buffer::Ptr &buf, bool try_flush) {
    SPDLOG_DEBUG("socket {0} send {1} bytes data", id_, buf->GetContentSize());

    return Send(buf, nullptr, 0, try_flush);
}

ssize_t Socket::SendTo(Buffer::Ptr &buf, const char *host, uint16_t port, bool try_flush) {
    SPDLOG_DEBUG("socket {0} send {1} bytes data to {2}:{3}", id_, buf->GetContentSize(), host, port);

    sockaddr_in addr{};
    auto addr_len = sizeof(addr);
    memset(&addr, 0, addr_len);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    return Send(buf, (sockaddr *) &addr, addr_len, try_flush);
}

ssize_t Socket::Send(Buffer::Ptr &buf, sockaddr *addr, socklen_t addr_len, bool try_flush) {
    if (socket_fd_ <= 0) {
        return 0;
    }

    auto size = buf->GetContentSize();
    if (size <= 0) {
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(send_queue_mutex_);
        auto data = std::make_shared<BufferSock>(buf, addr, addr_len);
        send_queue_.push_back(data);
    }

    if (try_flush && available_send_) {
        SPDLOG_DEBUG("socket was available to sent, flush it");
        Flush(false);
    } else {
        SPDLOG_DEBUG("socket was not available to sent, just wait");
    }

    return size;
}

void Socket::SetSendTimeOutSecond(uint32_t seconds) {
//    socket_->setSendTimeOutSecond(seconds);
}

void Socket::Close() {
    SPDLOG_DEBUG("socket {0} close", id_);

    if (socket_fd_) {
        UnRegisterEvent();
        close(socket_fd_);
        socket_fd_ = 0;
    }

    socket_type_ = SocketType::Invalid;
    available_send_ = false;

    send_queue_.clear();
    sending_buffer_.reset();

    try {
        closed_callback_();
    } catch (std::exception &ex) {
        SPDLOG_ERROR("socket {0} closed callback raised exception '{1}'", id_, ex.what());
    }
}

std::string Socket::GetLocalIp() {
//    return socket_->get_local_ip();
    return "";
}

uint16_t Socket::GetLocalPort() {
//    return socket_->get_local_port();
    return 0;
}

std::string Socket::GetPeerIp() {
//    return socket_->get_peer_ip();
    return "";
}

uint16_t Socket::GetPeerPort() {
//    return socket_->get_peer_port();
    return 0;
}

void Socket::RegisterEvent() {
    int event = 0;
    if (socket_type_ == SocketType::TcpServer) {
        event = Event_Readable | Event_Error;
    } else if (socket_type_ == SocketType::TcpClient || socket_type_ == SocketType::Udp) {
        event = Event_Readable | Event_Writable | Event_Error;
    }

    auto weak_self = weak_from_this();
    poll_thread_->AddEvent(socket_fd_, event, [weak_self](int event) {
        auto strong_self = weak_self.lock();
        if (strong_self == nullptr) {
            return;
        }

        strong_self->OnPollEvent(event);
    });
}

void Socket::StartWritableEvent() {
    SPDLOG_DEBUG("socket {0} start writable event", id_);
    available_send_ = false;

    auto event = Event_Readable | Event_Writable | Event_Error;
    poll_thread_->ModifyEvent(socket_fd_, event, nullptr);
}

void Socket::StopWritableEvent() {
    SPDLOG_DEBUG("socket {0} stop writable event", id_);
    auto event = Event_Readable | Event_Error;
    poll_thread_->ModifyEvent(socket_fd_, event, nullptr);
}

void Socket::UnRegisterEvent() {
    poll_thread_->DelEvent(socket_fd_, nullptr);
}

void Socket::OnPollEvent(int event) {
    if (event & Event_Readable) {
        if (socket_type_ == SocketType::TcpServer) {
            OnAcceptEvent();
        } else {
            OnReadableEvent();
        }
    }

    if (event & Event_Writable) {
        OnWritableEvent();
    }

    if (event & Event_Error) {
        OnErrorEvent();
    }
}

void Socket::OnAcceptEvent() {
    SPDLOG_DEBUG("socket {0} received accept event", id_);

    sockaddr_in remote_addr{};
    memset(&remote_addr, 0, sizeof(remote_addr));
    auto sin_size = sizeof(sockaddr_in);
    auto client_fd = accept(socket_fd_, (sockaddr *) (&remote_addr), (socklen_t *) &sin_size);
    if (client_fd < 0) {
        SPDLOG_ERROR("socket {0} server accept failed with error {1}, description '{2}'",
                     id_, errno, strerror(errno));
        return;
    }

    SocketUtils::setNoSigpipe(client_fd);
    SocketUtils::setNoBlocked(client_fd);
    SocketUtils::setNoDelay(client_fd);
    SocketUtils::setSendBuf(client_fd, SOCKET_DEFAULT_BUF_SIZE);
    SocketUtils::setRecvBuf(client_fd, SOCKET_DEFAULT_BUF_SIZE);
    SocketUtils::setCloseWait(client_fd);
    SocketUtils::setCloExec(client_fd);

    auto client_socket = before_create_callback_();
    client_socket->socket_fd_ = client_fd;
    client_socket->socket_type_ = SocketType::TcpClient;
    client_socket->RegisterEvent();

    try {
        accept_callback_(client_socket, (sockaddr *) &remote_addr, (int) sin_size);
    } catch (std::exception &ex) {
        SPDLOG_ERROR("socket {0} accept callback raise exception '{1}'", id_, ex.what());
    }
}

void Socket::OnReadableEvent() {
    SPDLOG_DEBUG("socket {0} received readable event", id_);

    auto read_buffer = poll_thread_->GetSharedReadBuffer();

    auto data = read_buffer->GetWritableData();
    auto capacity = read_buffer->GetCapacity();

    sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    while (true) {
        memset(&addr, 0, addr_len);
        auto read_count = recvfrom(socket_fd_, data, capacity, 0, (sockaddr *) &addr, &addr_len);
        if (read_count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // read finished
                break;
            }

            SPDLOG_ERROR("socket {0} read failed with error {1}, description '{2}'",
                         id_, errno, strerror(errno));
            Close();

            return;
        } else if (read_count == 0) {
            SPDLOG_INFO("socket {0} received 0 bytes read event, the remote was disconnected", id_);
            Close();

            return;
        }

        SPDLOG_DEBUG("socket {0} received {1} bytes", id_, read_count);
        read_buffer->IncreaseContentSize(static_cast<int>(read_count));

        try {
            std::shared_ptr<Buffer> buffer = read_buffer;
            read_callback_(buffer, (sockaddr *) &addr, static_cast<int>(addr_len));
        } catch (std::exception &ex) {
            SPDLOG_ERROR("socket {0} read callback raise exception '{1}'", id_, ex.what());
        }
    }
}

void Socket::OnWritableEvent() {
    SPDLOG_DEBUG("socket {0} received writable event", id_);

    if (connecting_) {
        connecting_ = false;

        int err = -1;
        socklen_t len = sizeof(err);
        int ret = getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &err, &len);
        if (ret < 0) {
            SPDLOG_ERROR("socket {0} get socket error failed with error {1}, description '{2}'",
                         id_, errno, strerror(errno));
            try {
                connect_callback_(Socket_Connect_Failed);
            } catch (std::exception &ex) {
                SPDLOG_ERROR("socket {0} connect callback raise exception '{1}'", id_, ex.what());
            }

            Close();
        } else {
            try {
                connect_callback_(Success);
            } catch (std::exception &ex) {
                SPDLOG_ERROR("socket {0} connect callback raise exception '{1}'", id_, ex.what());
            }
        }

        return;
    }

    available_send_ = true;

    Flush(true);
}

void Socket::OnErrorEvent() {
    SPDLOG_WARN("socket {0} received error event", id_);

    int err = -1;
    socklen_t len = sizeof(err);
    int ret = getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &err, &len);
    if (ret < 0) {
        SPDLOG_ERROR("socket {0} get socket error failed with error {1}, description '{2}'",
                     id_, errno, strerror(errno));
        Close();
    } else {
        SPDLOG_INFO("socket {0} got socket error {1}", id_, err);
    }

    try {
        error_callback_(Success);
    } catch (std::exception &ex) {
        SPDLOG_ERROR("socket {0} error callback raise exception '{1}'", id_, ex.what());
    }
}

void Socket::Flush(bool by_poll_thread) {
    SPDLOG_DEBUG("socket {0} flush by poll thread {1}", id_, by_poll_thread);

    if (socket_fd_ <= 0) {
        SPDLOG_DEBUG("socket {0} has invalid socket fd", id_);
        return;
    }

    if (!available_send_) {
        SPDLOG_DEBUG("socket {0} was not available to send", id_);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(sending_buffer_mutex_);
        if (sending_buffer_ && sending_buffer_->IsFinished()) {
            auto sent_data = sending_buffer_->GetBuffer();
            sent_result_callback_(sent_data, true);
            sending_buffer_.reset();
        }

        if (sending_buffer_ == nullptr) {
            std::lock_guard<std::mutex> lock_queue(send_queue_mutex_);
            if (!send_queue_.empty()) {
                sending_buffer_ = send_queue_.front();
                send_queue_.pop_front();
            }
        }

        if (sending_buffer_ == nullptr) {
            SPDLOG_DEBUG("socket {0} has no data to send", id_);
            StopWritableEvent();

            return;
        }

        ssize_t sent_count;
        auto buffer = sending_buffer_->GetBuffer();

        while (true) {
            auto data = buffer->GetData() + sending_buffer_->GetOffset();
            auto size = buffer->GetContentSize() - sending_buffer_->GetOffset();
            if (size == 0) {
                break;
            }

            SPDLOG_DEBUG("socket {0} send data with {1} bytes", id_, size);

            if (socket_type_ == SocketType::Udp) {
                sent_count = ::sendto(socket_fd_, data, size, send_flags_,
                                      sending_buffer_->GetAddress(), sending_buffer_->GetAddressLength());
            } else {
                sent_count = ::send(socket_fd_, data, size, send_flags_);
            }

            if (sent_count < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // send buffer was full
                    break;
                }

                SPDLOG_ERROR("socket {0} send failed with error {1}, description '{2}'",
                             id_, errno, strerror(errno));
                try {
                    sent_result_callback_(buffer, false);
                } catch (std::exception &ex) {
                    SPDLOG_WARN("socket {0} sent result callback raise exception '{1}'", id_, ex.what());
                }
                // TODO: should close the connection here?
            } else {
                sending_buffer_->UpdateSentDataCount(sent_count);
            }
        }

        available_send_ = false;

        if (!by_poll_thread) {
            StartWritableEvent();
        }
    }
}
