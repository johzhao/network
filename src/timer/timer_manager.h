#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <map>

#include "timer.h"

class TimerManager {
public:
    static TimerManager *GetInstance();

    ~TimerManager();

    using TimerCallback = std::function<void()>;

public:
    long After(long microseconds, const TimerCallback &callback);

    void Cancel(long id);

private:
    TimerManager();

    void ClearStoppedTimers();

private:
    long next_timer_id_ = 1000;
    std::map<long, std::shared_ptr<Timer>> timer_map_;
};

#endif //TIMER_MANAGER_H
