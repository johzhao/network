add_library(socket STATIC
        socket.cpp
        poll_thread.cpp
        poll_thread_pool.cpp
        socket_utils.cpp
        session.cpp
        tcp_server.cpp
)
target_link_libraries(socket PRIVATE
        pthread
        spdlog
        utils
        fmt
)
