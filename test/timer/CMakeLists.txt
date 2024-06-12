add_executable(test_timer
        test_timer_manager.cpp
)
target_link_libraries(test_timer PRIVATE
        pthread
        spdlog
        gtest
        gtest_main
        timer
)
