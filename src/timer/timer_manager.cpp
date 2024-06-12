#include "timer_manager.h"

static TimerManager *instance = nullptr;

TimerManager *TimerManager::GetInstance() {
    if (instance == nullptr) {
        instance = new TimerManager();
    }

    return instance;
}

TimerManager::TimerManager() = default;

TimerManager::~TimerManager() {
    timer_map_.clear();
}

long TimerManager::After(long microseconds, const TimerCallback &callback) {
    auto timer_id = next_timer_id_++;
    auto timer = std::make_shared<Timer>(timer_id);
    timer->Start(microseconds, [this, callback, timer_id]() {
        auto it = timer_map_.find(timer_id);
        if (it == timer_map_.end()) {
            return;
        }

        callback();
    });

    timer_map_[timer_id] = timer;

    ClearStoppedTimers();

    return timer_id;
}

void TimerManager::Cancel(long id) {
    auto it = timer_map_.find(id);
    if (it == timer_map_.end()) {
        return;
    }

    it->second->Cancel();
    timer_map_.erase(it);
}

void TimerManager::ClearStoppedTimers() {
    for (auto &it: timer_map_) {
        if (it.second->IsStopped()) {
            timer_map_.erase(it.first);
        }
    }
}
