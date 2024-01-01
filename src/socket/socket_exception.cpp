#include "socket_exception.h"

SocketException::SocketException(ErrCode code, const std::string &msg, int custom_code)
        : code_(code), custom_code_(custom_code), msg_(msg) {
}

SocketException::~SocketException() = default;

SocketException::operator bool() const {
    return code_ != Err_success;
}

ErrCode SocketException::getErrCode() const {
    return code_;
}

int SocketException::getCustomCode() const {
    return custom_code_;
}

const char *SocketException::what() const noexcept {
    return msg_.c_str();
}
