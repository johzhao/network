add_executable(network
        main.cpp
)
target_link_libraries(network PRIVATE
        dl
        pthread
)
