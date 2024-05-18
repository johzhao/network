#ifndef BUFFER_SOCK_H
#define BUFFER_SOCK_H

#include <sys/socket.h>

#include "buffer.h"
#include "copy_buffer.h"

class BufferSock {
public:
    explicit BufferSock(std::shared_ptr<Buffer> &buffer, sockaddr *address, socklen_t addr_len);

    BufferSock(BufferSock &other) = delete;

    BufferSock operator=(BufferSock &other) = delete;

    ~BufferSock();

public:
    std::shared_ptr<Buffer> GetBuffer() const;

    bool ContainAddress() const;

    sockaddr *GetAddress() const;

    socklen_t GetAddressLength() const;

    ssize_t GetOffset() const;

    void UpdateSentDataCount(ssize_t count);

    bool IsFinished() const;

private:
    std::shared_ptr<CopyBuffer> buffer_ = nullptr;
    char *addr_ = nullptr;
    socklen_t addr_len_ = 0;
    ssize_t offset_ = 0;
};

#endif //BUFFER_SOCK_H
