#ifndef SOCKET_EXCEPTION_H
#define SOCKET_EXCEPTION_H

#include <exception>
#include <string>

typedef enum {
    Err_success = 0, //成功 success
    Err_eof, //eof
    Err_timeout, //超时 socket timeout
    Err_refused,//连接被拒绝 socket refused
    Err_reset,//连接被重置  socket reset
    Err_dns,//dns解析失败 dns resolve failed
    Err_shutdown,//主动关闭 socket shutdown
    Err_other = 0xFF,//其他错误 other error
} ErrCode;

class SocketException : public std::exception {
public:
    explicit SocketException(ErrCode code = Err_success, const std::string &msg = "", int custom_code = 0);

    ~SocketException() override;

public:
    operator bool() const;

    ErrCode getErrCode() const;

    int getCustomCode() const;

    const char *what() const noexcept override;

private:
    ErrCode code_;
    int custom_code_;
    std::string msg_;
};

#endif //SOCKET_EXCEPTION_H
