#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <vector>
#include <future>

#include "spdlog/spdlog.h"

#include "socket/poll_thread.h"
#include "socket/socket.h"

static constexpr int kServerPort = 1234;

static std::vector<std::shared_ptr<Socket>> connections;

void HandleNewConnection(std::shared_ptr<Socket> &sock) {
    SPDLOG_INFO("server received connection");

    sock->SetOnReadCallback([sock](Buffer::Ptr &buf, sockaddr *addr, int addr_len) {
        auto data = std::make_shared<CopyBuffer>(buf);
        SPDLOG_INFO("server received data '{0}'", data->GetData());

        if (data->GetContentSize()) {
            std::shared_ptr<Buffer> data_to_send = data;
            sock->Send(data_to_send);
        }
    });

    connections.push_back(sock);
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

    auto server_socket = std::make_shared<Socket>("server", poll_thread);
    error_code = server_socket->Listen(kServerPort);
    if (error_code != Success) {
        return error_code;
    }
    server_socket->SetOnAcceptCallback([](std::shared_ptr<Socket> &sock, sockaddr *addr, int addr_len) {
        HandleNewConnection(sock);
    });

    SPDLOG_INFO("create the tcp client");

    auto client_socket = std::make_shared<Socket>("client", poll_thread);
    client_socket->Connect("127.0.0.1", kServerPort, [client_socket](ErrorCode error_code) {
        SPDLOG_INFO("client connect to server result was {0}", int(error_code));
        if (error_code != Success) {
            return;
        }

        SPDLOG_INFO("client send data to server");

        auto data = "abcdefg";
        auto buffer = std::make_shared<Buffer>(data, strlen(data));
        client_socket->Send(buffer);
    });
    client_socket->SetOnReadCallback([client_socket](Buffer::Ptr &buf, sockaddr *addr, int addr_len) {
        auto data = std::make_shared<CopyBuffer>(buf);
        SPDLOG_INFO("client received data: '{0}'", data->GetData());

        auto _ = std::async(std::launch::deferred, [client_socket]() {
            client_socket->Close();
        });
    });

    sleep(1);

    return Success;
}

int main(int argc, char *argv[]) {
    auto logger = spdlog::default_logger();
    logger->set_level(spdlog::level::trace);

    int ret = test_tcp() == Success ? 0 : -1;
    return ret;
}
