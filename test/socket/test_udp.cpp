#include <memory>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "spdlog/spdlog.h"

#include "socket/poll_thread.h"
#include "socket/socket.h"

static constexpr int kServerPort = 1234;
static constexpr int kClientPort = 5678;
static const char *kRemoteAddr = "127.0.0.1";

ErrorCode test_udp() {
    ErrorCode error_code;

    SPDLOG_INFO("create the poll thread");

    auto poll_thread = std::make_shared<PollThread>(0);
    error_code = poll_thread->Initialize();
    if (error_code != Success) {
        return error_code;
    }

    SPDLOG_INFO("create the udp server");

    auto server_socket = std::make_shared<Socket>("0", poll_thread);
    error_code = server_socket->BindUdpSock(kServerPort);
    if (error_code != Success) {
        return error_code;
    }
    server_socket->SetOnReadCallback([server_socket](Buffer::Ptr &buf, sockaddr *addr, int addr_len) {
        auto data = std::make_shared<CopyBuffer>(buf);
        SPDLOG_INFO("server received data '{0}'", data->GetData());

        server_socket->Send(buf, addr, addr_len, true);
    });

    SPDLOG_INFO("create the udp client");

    auto client_socket = std::make_shared<Socket>("1", poll_thread);
    error_code = client_socket->BindUdpSock(kClientPort);
    if (error_code != Success) {
        return error_code;
    }
    client_socket->SetOnReadCallback([](Buffer::Ptr &buf, sockaddr *addr, int addr_len) {
        auto data = std::make_shared<CopyBuffer>(buf);
        SPDLOG_INFO("client received data: '{0}'", data->GetData());
    });

    SPDLOG_INFO("client send data to server");

    sockaddr_in addr{};
    auto addr_len = sizeof(addr);
    memset(&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kServerPort);
    addr.sin_addr.s_addr = inet_addr(kRemoteAddr);

    auto data = "abcdefg";
    auto buffer = std::make_shared<Buffer>(data, strlen(data));
    client_socket->SendTo(buffer, kRemoteAddr, kServerPort);

    sleep(1);

    return Success;
}

int main(int argc, char *argv[]) {
    auto logger = spdlog::default_logger();
    logger->set_level(spdlog::level::trace);

    int ret = test_udp() == Success ? 0 : -1;
    return ret;
}
