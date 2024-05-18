#include "poll_thread_pool.h"

static PollThreadPool *instance_ = nullptr;

PollThreadPool::PollThreadPool() = default;

PollThreadPool::~PollThreadPool() = default;

ErrorCode PollThreadPool::Initialize(int pool_size) {
    instance_ = new PollThreadPool();

    if (pool_size < 0) {
        pool_size = static_cast<int>(std::thread::hardware_concurrency());
    }

    for (int i = 0; i < pool_size; ++i) {
        auto poll_thread = std::make_shared<PollThread>(i);
        poll_thread->Initialize();
        instance_->pool_.push_back(poll_thread);
    }

    return Success;
}

PollThreadPool *PollThreadPool::GetInstance() {
    return instance_;
}

std::shared_ptr<PollThread> PollThreadPool::GetPollThread() {
    auto index = ++index_ % pool_.size();
    return pool_[index];
}
