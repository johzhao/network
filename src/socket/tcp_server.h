#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <cstdint>
#include <functional>
#include <string>
#include <memory>

#include "error_code.h"
#include "session.h"

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    explicit TcpServer(std::string id, std::shared_ptr<PollThread> &poll_thread);

    ~TcpServer();

    using SessionCreator = std::function<std::shared_ptr<Session>(const std::string &id,
                                                                  std::shared_ptr<Socket> &socket)>;

    using NewSessionCallback = std::function<void(std::shared_ptr<Session> &)>;

public:
    void SetSessionCreator(SessionCreator callback);

    void SetNewSessionCallback(NewSessionCallback callback);

    ErrorCode Start(uint16_t port, const std::string &host = "0.0.0.0", int backlog = 1024);

    void Stop();

private:
    void SetListenSocketCallback();

    void OnError(ErrorCode error_code);

    void OnAccepted(std::shared_ptr<Socket> &sock, sockaddr *addr, int addr_len);

private:
    std::string id_;
    std::shared_ptr<PollThread> poll_thread_;
    std::shared_ptr<Socket> listen_socket_ = nullptr;
    SessionCreator session_creator_;
    NewSessionCallback new_session_callback_;
    int next_session_index_ = 0;
};

#endif //TCP_SERVER_H
