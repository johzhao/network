#ifndef TIMER_H
#define TIMER_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

class Timer {
public:
    explicit Timer(long id);

    ~Timer();

public:
    long GetId() const;

    void Start(long microseconds, const std::function<void()> &callback);

    void Cancel();

    bool IsStopped() const;

private:
    void RunLoop();

private:
    long id_ = 0;
    long microseconds_ = 0;
    std::function<void()> callback_;
    std::thread work_thread_;
    std::atomic<bool> stop_flag_{false};
    std::mutex mutex_;
    std::condition_variable condition_variable_;
};

#endif //TIMER_H
