#include <csignal>
#include <memory>
#include <netinet/in.h>
#include <vector>

#include "spdlog/spdlog.h"

#include "socket/socket.h"
#include "socket/poll_thread.h"

static constexpr int kServerPort = 10000;

bool stop = false;
std::mutex mutex;
std::condition_variable condition;

class Session {
public:
    explicit Session(std::shared_ptr<Socket> &socket)
            : socket_(socket) {
        socket_->SetOnReadCallback([this](Buffer::Ptr &buf, sockaddr *, int) {
            OnReceiveData(buf);
        });
    }

public:
    void OnReceiveData(Buffer::Ptr &buf) {
        socket_->Send(buf);
    }

private:
    std::shared_ptr<Socket> socket_;
};

static std::vector<std::shared_ptr<Session>> sessions;

void HandleNewConnection(std::shared_ptr<Socket> &sock) {
    SPDLOG_INFO("server received connection {0}", sock->GetId());

    auto session = std::make_shared<Session>(sock);
    sessions.push_back(session);
}

ErrorCode test_tcp() {
    ErrorCode error_code;

    SPDLOG_INFO("create the poll thread");

    auto poll_thread = std::make_shared<PollThread>(0);
    error_code = poll_thread->Initialize();
    if (error_code != Success) {
        return error_code;
    }

    SPDLOG_INFO("create the tcp server");

    auto server_socket = std::make_shared<Socket>("1", poll_thread);
    error_code = server_socket->Listen(kServerPort);
    if (error_code != Success) {
        return error_code;
    }
    server_socket->SetOnAcceptCallback([](std::shared_ptr<Socket> &sock, sockaddr *addr, int addr_len) {
        HandleNewConnection(sock);
    });

    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, []() {
        return stop;
    });

    sessions.clear();

    sleep(1);

    return Success;
}

void handleSignalEvent(int) {
    stop = true;
    condition.notify_all();
}

int main(int, char *[]) {
    auto logger = spdlog::default_logger();
    logger->set_level(spdlog::level::trace);

    signal(SIGTERM, handleSignalEvent);
    signal(SIGINT, handleSignalEvent);

    int ret = test_tcp() == Success ? 0 : -1;

    spdlog::shutdown();

    return ret;
}
