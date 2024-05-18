#ifndef POLL_THREAD_POOL_H
#define POLL_THREAD_POOL_H

#include <memory>
#include <vector>

#include "poll_thread.h"

class PollThreadPool {
public:
    ~PollThreadPool();

public:
    static ErrorCode Initialize(int pool_size = -1);

    static PollThreadPool *GetInstance();

    std::shared_ptr<PollThread> GetPollThread();

private:
    explicit PollThreadPool();

private:
    std::vector<std::shared_ptr<PollThread>> pool_;
    int index_ = 0;
};

#endif //POLL_THREAD_POOL_H
