#ifndef SESSION_H
#define SESSION_H

#include <functional>
#include <memory>
#include <string>
#include <socket/socket.h>

#include "utils/buffer.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(std::string id, std::shared_ptr<Socket> &socket);

    virtual ~Session();

    using ErrorCallback = std::function<void(ErrorCode error_code)>;

    using DisconnectedCallback = std::function<void()>;

public:
    const std::string &GetId() const;

    void SetAddress(sockaddr *addr, int addr_len);

    void SetErrorCallback(ErrorCallback callback);

    void SetDisconnectedCallback(DisconnectedCallback callback);

    void Send(std::shared_ptr<Buffer> &buf);

    void Close();

protected:
    virtual void OnReceived(std::shared_ptr<Buffer> &buf, sockaddr *addr, int addr_len);

    virtual void OnSentResult(std::shared_ptr<Buffer> &buffer, bool send_success);

    virtual void OnError(ErrorCode error_code);

    virtual void OnClosed();

    void RegisterSocketCallback();

    void UnRegisterSocketCallback();

protected:
    std::string id_;
    char *addr_ = nullptr;
    int addr_len_ = 0;
    std::shared_ptr<Socket> socket_;
    ErrorCallback error_callback_;
    DisconnectedCallback disconnected_callback_;
};

#endif //SESSION_H
