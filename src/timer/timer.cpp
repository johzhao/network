#include "timer.h"

#include "spdlog/spdlog.h"

Timer::Timer(long id)
        : id_(id) {
}

Timer::~Timer() {
    Cancel();
}

long Timer::GetId() const {
    return id_;
}

void Timer::Start(long microseconds, const std::function<void()> &callback) {
    microseconds_ = microseconds;
    callback_ = callback;
    work_thread_ = std::thread([this]() {
        RunLoop();
    });
}

void Timer::Cancel() {
    stop_flag_ = true;
    condition_variable_.notify_one();

    if (work_thread_.joinable()) {
        work_thread_.join();
    }
}

bool Timer::IsStopped() const {
    return stop_flag_;
}

void Timer::RunLoop() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_variable_.wait_for(lock, std::chrono::microseconds(microseconds_), [this]() {
        return bool(stop_flag_);
    });

    if (stop_flag_) {
        return;
    }

    try {
        callback_();
    } catch (std::exception &ex) {
        SPDLOG_ERROR("timer {} callback raised exception {}", id_, ex.what());
    }

    stop_flag_ = true;
}
