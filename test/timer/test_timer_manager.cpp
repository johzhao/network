#include <semaphore.h>

#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

#include "timer/timer_manager.h"

sem_t sem;

void TimerCallback() {
    SPDLOG_INFO("in timer callback");
    sem_post(&sem);
}

TEST(TestTimerSuite, TestSetTimer) {
    sem_init(&sem, 0, 0);

    auto timer_manager = TimerManager::GetInstance();

    SPDLOG_INFO("before create timer");

    auto timer_id = timer_manager->After(10 * 1000, TimerCallback);
    EXPECT_GT(timer_id, 0);

    sem_wait(&sem);
    SPDLOG_INFO("after wait");

    sem_destroy(&sem);
}

TEST(TestTimerSuite, TestCancelTimer) {
    auto timer_manager = TimerManager::GetInstance();
    auto timer_id = timer_manager->After(10 * 1000, TimerCallback);
    EXPECT_GT(timer_id, 0);

    timer_manager->Cancel(timer_id);
}
