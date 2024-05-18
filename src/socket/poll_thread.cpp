#include "poll_thread.h"

#include <unistd.h>

#include "spdlog/spdlog.h"

static constexpr int kMaxEpollEventCount = 64;
static constexpr int kSharedReadBufferSize = 1024 * 1024;

PollThread::PollThread(int id)
        : id_(id) {
}

PollThread::~PollThread() {
    Release();

    SPDLOG_INFO("poll thread {0} was de-constructed", id_);
}

ErrorCode PollThread::Initialize() {
    if (epoll_fd_) {
        return Already_Initialized;
    }

    events_ = new epoll_event[kMaxEpollEventCount];
    shared_read_buffer_ = std::make_shared<MutableBuffer>(kSharedReadBufferSize);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        SPDLOG_ERROR("poll thread {0} create epoll failed with error {1}, reason: '{2}'",
                     id_, errno, strerror(errno));
        return Create_Epoll_Failed;
    }

    epoll_fd_ = epoll_fd;
    work_thread_ = std::thread([this]() {
        RunLoop();
    });

    return Success;
}

void PollThread::Release() {
    stop_flag_ = true;
    if (work_thread_.joinable()) {
        work_thread_.join();
    }

    if (epoll_fd_) {
        close(epoll_fd_);
        epoll_fd_ = 0;
    }

    event_map_.clear();
    shared_read_buffer_.reset();
    delete[] events_;
    events_ = nullptr;

    SPDLOG_INFO("poll thread {0} was released", id_);
}

ErrorCode PollThread::AddEvent(int fd, int events, PollEventCallback callback) {
    std::unique_lock<std::mutex> lock(mutex_);

    epoll_event ev{};
    ev.events = ToPollEvents(events);
    ev.data.fd = fd;
    int rc = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    if (rc) {
        SPDLOG_ERROR("poll thread {0} epoll_ctl add event failed with error: {1}, reason: '{2}'",
                     id_, errno, strerror(errno));
        return Add_Epoll_Event_Failed;
    }

    event_map_[fd] = std::make_shared<PollEventCallback>(std::move(callback));

    return Success;
}

ErrorCode PollThread::DelEvent(int fd, const PollCompleteCallback &callback) {
    std::unique_lock<std::mutex> lock(mutex_);

    ErrorCode ret = Success;

    epoll_event ev{};
    ev.events = 0;
    ev.data.fd = fd;
    int rc = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev);
    if (rc) {
        SPDLOG_ERROR("poll thread {0} epoll_ctl delete event failed with error: {1}, reason: '{2}'",
                     id_, errno, strerror(errno));
        ret = Delete_Epoll_Event_Failed;
    }

    event_map_.erase(fd);

    if (callback) {
        try {
            callback(rc == 0);
        } catch (std::exception &ex) {
            SPDLOG_ERROR("poll thread {0} delete event callback raise exception '{1}'", id_, ex.what());
        }
    }

    return ret;
}

ErrorCode PollThread::ModifyEvent(int fd, int events, const PollCompleteCallback &callback) {
    std::unique_lock<std::mutex> lock(mutex_);

    ErrorCode ret = Success;

    epoll_event ev{};
    ev.events = ToPollEvents(events);
    ev.data.fd = fd;
    int rc = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
    if (rc) {
        SPDLOG_ERROR("poll thread {0} epoll_ctl modify event failed with error: {1}, reason: '{2}'",
                     id_, errno, strerror(errno));
        ret = Modify_Epoll_Event_Failed;
    }

    if (callback) {
        try {
            callback(rc == 0);
        } catch (std::exception &ex) {
            SPDLOG_ERROR("poll thread {0} modify event callback raise exception '{1}'", id_, ex.what());
        }
    }

    return ret;
}

std::shared_ptr<MutableBuffer> PollThread::GetSharedReadBuffer() const {
    shared_read_buffer_->Reset();
    return shared_read_buffer_;
}

void PollThread::RunLoop() {
    do {
        int nfds = epoll_wait(epoll_fd_, events_, kMaxEpollEventCount, 1000);
        if (nfds < 0) {
            SPDLOG_WARN("epoll wait failed with error: {0}, description: '{1}'", errno, strerror(errno));
            continue;
        }

        if (stop_flag_) {
            break;
        }

        SPDLOG_TRACE("epoll {0} thread return with {1} events", id_, nfds);

        for (int n = 0; n < nfds; ++n) {
            auto fd = events_[n].data.fd;
            auto it = event_map_.find(fd);
            if (it == event_map_.end()) {
                DelEvent(fd, nullptr);
                continue;
            }

            auto callback = it->second;
            auto events = events_[n].events;
            SPDLOG_DEBUG("epoll {0} thread received event by index {1} was 0x{2:08X}", id_, n, events);

            try {
                callback->operator()(FromPollEvents(events));
            } catch (std::exception &ex) {
                SPDLOG_ERROR("epoll {0} thread event callback raise exception '{1}'", id_, ex.what());
            }
        }
    } while (!stop_flag_);
}

uint32_t PollThread::ToPollEvents(int events) {
    uint32_t result = 0;

    if (events & Event_Readable) {
        result |= EPOLLIN;
    }

    if (events & Event_Writable) {
        result |= EPOLLOUT;
    }

    if (events & Event_Error) {
        result |= (EPOLLHUP | EPOLLERR);
    }

    if (events & Event_ET) {
        result |= EPOLLET;
    }

    return result;
}

int PollThread::FromPollEvents(uint32_t events) {
    int result = 0;

    if (events & EPOLLIN) {
        result |= Event_Readable;
    }

    if (events & EPOLLOUT) {
        result |= Event_Writable;
    }

    if (events & EPOLLHUP || events & EPOLLERR) {
        result |= Event_Error;
    }

    return result;
}
