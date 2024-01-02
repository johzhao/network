#ifndef EVENT_POLLER_H
#define EVENT_POLLER_H

#include <functional>
#include <map>
#include <memory>
#include <thread>

#include "utils/buffer.h"

typedef enum {
    Event_Read = 1 << 0, //读事件
    Event_Write = 1 << 1, //写事件
    Event_Error = 1 << 2, //错误事件
    Event_LT = 1 << 3,//水平触发
} Poll_Event;

class EventPoller {
public:
    using Ptr = std::shared_ptr<EventPoller>;

    using PollEventCB = std::function<void(int event)>;

    explicit EventPoller();

    ~EventPoller();

public:
    void Run();

    void Stop();

    int AddEvent(int fd, int event, PollEventCB cb);

    int ModifyEvent(int fd, int event);

    int DelEvent(int fd);

    Buffer::Ptr GetSharedBuffer();

private:
    void RunLoop();

private:
    int epoll_fd_ = -1;
    std::map<int, std::shared_ptr<PollEventCB> > event_map_;
    std::thread poll_thread_;
    bool stop_flag_ = false;
    std::weak_ptr<Buffer> _shared_buffer;
};

#endif //EVENT_POLLER_H
