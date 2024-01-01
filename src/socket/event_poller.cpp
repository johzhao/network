#include "event_poller.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>

#include "loguru/loguru.hpp"
#include "socket_utils.h"

#define toEpoll(event)        (((event) & Event_Read)  ? EPOLLIN : 0) \
                            | (((event) & Event_Write) ? EPOLLOUT : 0) \
                            | (((event) & Event_Error) ? (EPOLLHUP | EPOLLERR) : 0) \
                            | (((event) & Event_LT)    ? 0 : EPOLLET)

#define toPoller(epoll_event)     (((epoll_event) & EPOLLIN) ? Event_Read   : 0) \
                                | (((epoll_event) & EPOLLOUT) ? Event_Write : 0) \
                                | (((epoll_event) & EPOLLHUP) ? Event_Error : 0) \
                                | (((epoll_event) & EPOLLERR) ? Event_Error : 0)

#define EPOLL_SIZE 64

EventPoller::EventPoller() {
    epoll_fd_ = epoll_create(EPOLL_SIZE);
    if (epoll_fd_ < 0) {
        LOG_F(ERROR, "epoll create failed with error %d, message '%s'", errno, strerror(errno));
        throw std::runtime_error(strerror(errno));
    }
}

EventPoller::~EventPoller() {
    close(epoll_fd_);

    event_map_.clear();
}

void EventPoller::Run() {
    Stop();

    poll_thread_ = std::thread([this]() {
        this->RunLoop();
    });
}

void EventPoller::Stop() {
    stop_flag_ = true;
    poll_thread_.join();
}

int EventPoller::AddEvent(int fd, int event, PollEventCB cb) {
    if (cb == nullptr) {
        LOG_F(ERROR, "poll event callback is null");
        return -1;
    }

    epoll_event ev{};
    ev.events = toEpoll(event);
    ev.data.fd = fd;
    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret) {
        LOG_F(ERROR, "add event failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    event_map_.emplace(fd, std::make_shared<PollEventCB>(std::move(cb)));

    return 0;
}

int EventPoller::ModifyEvent(int fd, int event) {
    epoll_event ev{};
    ev.events = toEpoll(event);
    ev.data.fd = fd;
    auto ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
    if (ret) {
        LOG_F(ERROR, "modify event failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    return 0;
}

int EventPoller::DelEvent(int fd) {
    auto ret = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    if (ret) {
        LOG_F(ERROR, "delete event failed with error %d, message '%s'", errno, strerror(errno));
        return ret;
    }

    event_map_.erase(fd);

    return 0;
}

Buffer::Ptr EventPoller::GetSharedBuffer() {
    auto ret = _shared_buffer.lock();
    if (!ret) {
        //预留一个字节存放\0结尾符
        ret = std::shared_ptr<Buffer>();
        ret->SetCapacity(1 + SOCKET_DEFAULT_BUF_SIZE);
        _shared_buffer = ret;
    }

    return ret;
}

void EventPoller::RunLoop() {
    epoll_event events[EPOLL_SIZE];
    while (!stop_flag_) {
        int ret = epoll_wait(epoll_fd_, events, EPOLL_SIZE, -1);
        if (ret < 0) {
            continue;
        }
        for (int i = 0; i < ret; ++i) {
            auto ev = events[i];
            int fd = ev.data.fd;
            auto it = event_map_.find(fd);
            if (it == event_map_.end()) {
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
                continue;
            }

            try {
                auto callback = it->second;
                (*callback)(toPoller(ev.events));
            } catch (std::exception &ex) {
                LOG_F(ERROR, "event callback failed with exception '%s'", ex.what());
            }
        }
    }
}
