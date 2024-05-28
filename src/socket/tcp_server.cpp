#include "tcp_server.h"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

TcpServer::TcpServer(std::string id, std::shared_ptr<PollThread> &poll_thread)
        : id_(std::move(id)), poll_thread_(poll_thread) {
    SetSessionCreator(nullptr);
    SetNewSessionCallback(nullptr);
}

TcpServer::~TcpServer() {
    Stop();
}

void TcpServer::SetSessionCreator(SessionCreator callback) {
    if (callback == nullptr) {
        session_creator_ = [](const std::string &id, std::shared_ptr<Socket> &socket) {
            return std::make_shared<Session>(id, socket);
        };
    } else {
        session_creator_ = std::move(callback);
    }
}

void TcpServer::SetNewSessionCallback(TcpServer::NewSessionCallback callback) {
    if (callback == nullptr) {
        new_session_callback_ = [](std::shared_ptr<Session> &) {};
    } else {
        new_session_callback_ = std::move(callback);
    }
}

ErrorCode TcpServer::Start(uint16_t port, const std::string &host, int backlog) {
    ErrorCode error_code;
    if (listen_socket_) {
        return Already_Initialized;
    }

    listen_socket_ = std::make_shared<Socket>(id_, poll_thread_);
    error_code = listen_socket_->Initialize(SocketType::TcpServer, true);
    if (error_code != Success) {
        SPDLOG_ERROR("tcp server {} initialize failed with error 0x{:08X}", id_, int(error_code));
        return error_code;
    }

    SetListenSocketCallback();

    error_code = listen_socket_->Bind(port, host);
    if (error_code != Success) {
        SPDLOG_ERROR("tcp server {} bind to {}:{} failed with error 0x{:08X}", id_, host, port, int(error_code));
        return error_code;
    }

    error_code = listen_socket_->Listen(backlog);
    if (error_code != Success) {
        SPDLOG_ERROR("tcp server {} listen with backlog {} failed with error 0x{:08X}", id_, backlog, int(error_code));
        Stop();

        return error_code;
    }

    return Success;
}

void TcpServer::Stop() {
    if (listen_socket_) {
        listen_socket_->Close();

        listen_socket_.reset();
    }
}

void TcpServer::SetListenSocketCallback() {
    auto weak_self = weak_from_this();
    listen_socket_->SetOnErrorCallback([weak_self](ErrorCode error_code) {
        auto strong_self = weak_self.lock();
        if (strong_self == nullptr) {
            return;
        }
        strong_self->OnError(error_code);
    });

    listen_socket_->SetOnAcceptCallback([weak_self](std::shared_ptr<Socket> &sock, sockaddr *addr, int addr_len) {
        auto strong_self = weak_self.lock();
        if (strong_self == nullptr) {
            return;
        }
        strong_self->OnAccepted(sock, addr, addr_len);
    });

    next_session_index_ = 0;
}

void TcpServer::OnError(ErrorCode error_code) {
    SPDLOG_ERROR("tcp server {} was on error 0x{:08X}", id_, int(error_code));
}

void TcpServer::OnAccepted(std::shared_ptr<Socket> &sock, sockaddr *addr, int addr_len) {
    ++next_session_index_;
    auto session_id = fmt::format("{}-{}", id_, next_session_index_);
    auto session = session_creator_(session_id, sock);
    if (addr && addr_len) {
        session->SetAddress(addr, addr_len);
    }
    new_session_callback_(session);
}
