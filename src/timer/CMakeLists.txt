add_library(timer STATIC
        timer_manager.cpp
        timer.cpp
)
target_link_libraries(timer PRIVATE
        pthread
        spdlog
        fmt
)
