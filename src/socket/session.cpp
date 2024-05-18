#include "session.h"

#include "spdlog/spdlog.h"

Session::Session(std::string id, std::shared_ptr<Socket> &socket)
        : id_(std::move(id)), socket_(socket) {
    SetErrorCallback(nullptr);
    SetDisconnectedCallback(nullptr);

    RegisterSocketCallback();
}

Session::~Session() {
    Close();

    delete[] addr_;
}

const std::string &Session::GetId() const {
    return id_;
}

void Session::SetAddress(sockaddr *addr, int addr_len) {
    delete[] addr_;
    addr_ = nullptr;

    if (addr && addr_len) {
        addr_ = new char[addr_len];
        memcpy(addr_, addr, addr_len);
        addr_len_ = addr_len;
    }
}

void Session::SetErrorCallback(ErrorCallback callback) {
    if (callback == nullptr) {
        error_callback_ = [](ErrorCode) {};
    } else {
        error_callback_ = std::move(callback);
    }
}

void Session::SetDisconnectedCallback(DisconnectedCallback callback) {
    if (callback == nullptr) {
        disconnected_callback_ = []() {};
    } else {
        disconnected_callback_ = std::move(callback);
    }
}

void Session::Send(std::shared_ptr<Buffer> &buf) {
    if (addr_) {
        socket_->SendTo(buf, addr_, addr_len_);
    } else {
        socket_->Send(buf);
    }
}

void Session::Close() {
    socket_->Close();
}

void Session::OnReceived(std::shared_ptr<Buffer> &buf, sockaddr *addr, int addr_len) {
    // Implement by sub-class
}

void Session::OnSentResult(std::shared_ptr<Buffer> &buffer, bool send_success) {
    // Implement by sub-class
}

void Session::OnError(ErrorCode error_code) {
    try {
        error_callback_(error_code);
    } catch (std::exception &ex) {
        SPDLOG_ERROR("session {0} error callback raised exception '{1}'", id_, ex.what());
    }
}

void Session::OnClosed() {
    UnRegisterSocketCallback();

    try {
        disconnected_callback_();
    } catch (std::exception &ex) {
        SPDLOG_ERROR("session {0} closed callback raise exception '{1}'", id_, ex.what());
    }
}

void Session::RegisterSocketCallback() {
    auto weak_self = weak_from_this();
    socket_->SetOnReadCallback([weak_self](std::shared_ptr<Buffer> &buf, sockaddr *addr, int addr_len) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        strong_self->OnReceived(buf, addr, addr_len);
    });
    socket_->SetOnSentResultCallback([weak_self](std::shared_ptr<Buffer> &buf, bool send_success) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        strong_self->OnSentResult(buf, send_success);
    });
    socket_->SetOnErrorCallback([weak_self](ErrorCode error_code) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        strong_self->OnError(error_code);
    });
}

void Session::UnRegisterSocketCallback() {
    socket_->SetOnReadCallback(nullptr);
    socket_->SetOnSentResultCallback(nullptr);
    socket_->SetOnErrorCallback(nullptr);
    socket_->SetOnClosedCallback(nullptr);
}
