#ifndef POLL_THREAD_H
#define POLL_THREAD_H

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <thread>

#include "error_code.h"
#include "utils/mutable_buffer.h"

enum PollEvent {
    Event_Readable = 1 << 0,
    Event_Writable = 1 << 1,
    Event_Error = 1 << 2,
    Event_ET = 1 << 3,
};

class PollThread : public std::enable_shared_from_this<PollThread> {
public:
    explicit PollThread(int id);

    ~PollThread();

    using PollEventCallback = std::function<void(int event)>;

    using PollCompleteCallback = std::function<void(bool success)>;

public:
    ErrorCode Initialize();

    void Release();

    /**
     * 添加事件监听
     * @param fd 监听的文件描述符
     * @param events 事件类型，例如 Event_Read | Event_Write
     * @param cb 事件回调functional
     * @return -1:失败，0:成功
     */
    ErrorCode AddEvent(int fd, int events, PollEventCallback cb);

    /**
     * 删除事件监听
     * @param fd 监听的文件描述符
     * @param cb 删除成功回调functional
     * @return -1:失败，0:成功
     */
    ErrorCode DelEvent(int fd, const PollCompleteCallback &cb = nullptr);

    /**
     * 修改监听事件类型
     * @param fd 监听的文件描述符
     * @param events 事件类型，例如 Event_Read | Event_Write
     * @return -1:失败，0:成功
     */
    ErrorCode ModifyEvent(int fd, int events, const PollCompleteCallback &cb = nullptr);

    std::shared_ptr<MutableBuffer> GetSharedReadBuffer() const;

private:
    void RunLoop();

    static uint32_t ToPollEvents(int events);

    static int FromPollEvents(uint32_t events);

private:
    int id_ = 0;
    std::mutex mutex_;
    int epoll_fd_ = 0;
    epoll_event *events_ = nullptr;
    bool stop_flag_ = false;
    std::map<int, std::shared_ptr<PollEventCallback>> event_map_;
    std::shared_ptr<MutableBuffer> shared_read_buffer_ = nullptr;
    std::thread work_thread_;
};

#endif //POLL_THREAD_H
