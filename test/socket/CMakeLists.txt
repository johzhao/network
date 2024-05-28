add_executable(test_udp
        test_udp.cpp
)
target_link_libraries(test_udp PRIVATE
        dl
        pthread
        spdlog
        socket
        utils
)
add_test(NAME test_udp COMMAND test_udp)

add_executable(test_tcp
        test_tcp.cpp
)
target_link_libraries(test_tcp PRIVATE
        dl
        pthread
        spdlog
        fmt
        socket
        utils
)
add_test(NAME test_tcp COMMAND test_tcp)

add_executable(test_tcp_server
        test_tcp_server.cpp
)
target_link_libraries(test_tcp_server PRIVATE
        dl
        pthread
        spdlog
        socket
        utils
)
add_test(NAME test_tcp_server COMMAND test_tcp_server)
