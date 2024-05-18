#include <csignal>
#include <memory>
#include <netinet/in.h>
#include <vector>

#include "spdlog/spdlog.h"

#include "socket/socket.h"
#include "socket/poll_thread.h"
#include "socket/poll_thread_pool.h"

static constexpr int kServerPort = 10000;
static constexpr int kPackageSize = 1024;

bool stop = false;
std::mutex mutex;
std::condition_variable condition;

class Session {
public:
    explicit Session(std::shared_ptr<Socket> &socket)
            : socket_(socket),
              buffer_(std::make_shared<MutableBuffer>(1024 * 1024)) {
        socket_->SetOnReadCallback([this](Buffer::Ptr &buf, sockaddr *, int) {
            OnReceiveData(buf);
        });
    }

public:
    void OnReceiveData(Buffer::Ptr &buf) {
        buffer_->AppendData(buf->GetData(), buf->GetContentSize());
        if (buffer_->GetContentSize() < kPackageSize) {
            return;
        }

        auto data = std::make_shared<CopyBuffer>(buffer_->GetData(), kPackageSize);
        buffer_->ConsumeData(kPackageSize);

        std::shared_ptr<Buffer> buffer = data;
        socket_->Send(buffer);
    }

private:
    std::shared_ptr<Socket> socket_;
    std::shared_ptr<MutableBuffer> buffer_;
};

static std::vector<std::shared_ptr<Session>> sessions;

void HandleNewConnection(std::shared_ptr<Socket> &sock) {
    SPDLOG_INFO("server received connection {0}", sock->GetId());

    auto session = std::make_shared<Session>(sock);
    sessions.push_back(session);
}

std::shared_ptr<Socket> BeforeCreateCallback() {
    static int socket_id = 100;

    auto poll_thread = PollThreadPool::GetInstance()->GetPollThread();
    return std::make_shared<Socket>(++socket_id, poll_thread);
}

ErrorCode test_tcp() {
    ErrorCode error_code;

    SPDLOG_INFO("create the poll thread");

    PollThreadPool::Initialize(4);

    SPDLOG_INFO("create the tcp server");

    auto poll_thread = PollThreadPool::GetInstance()->GetPollThread();
    auto server_socket = std::make_shared<Socket>(1, poll_thread);
    server_socket->SetOnBeforeCreateCallback(BeforeCreateCallback);
    error_code = server_socket->Listen(kServerPort);
    if (error_code != Success) {
        return error_code;
    }
    server_socket->SetOnAcceptCallback([](std::shared_ptr<Socket> &sock) {
        HandleNewConnection(sock);
    });

    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, []() {
        return stop;
    });

    sleep(1);

    return Success;
}

void handleSignalEvent(int) {
    stop = true;
    condition.notify_all();
}

int main(int argc, char *argv[]) {
    auto logger = spdlog::default_logger();
    logger->set_level(spdlog::level::trace);

    signal(SIGTERM, handleSignalEvent);
    signal(SIGINT, handleSignalEvent);

    int ret = test_tcp() == Success ? 0 : -1;
    return ret;
}
