#include "buffer_sock.h"

#include <cstring>

BufferSock::BufferSock(std::shared_ptr<Buffer> &buffer, sockaddr *address, socklen_t addr_len)
        : addr_len_(addr_len) {
    if (buffer->GetContentSize() > 0) {
        buffer_ = std::make_shared<CopyBuffer>(buffer);
    }

    if (addr_len > 0) {
        addr_ = new char[addr_len_];
        memcpy(addr_, address, addr_len);
    }
}

BufferSock::~BufferSock() {
    delete[] addr_;
}

std::shared_ptr<Buffer> BufferSock::GetBuffer() const {
    return buffer_;
}

bool BufferSock::ContainAddress() const {
    return GetAddressLength() > 0;
}

sockaddr *BufferSock::GetAddress() const {
    return (sockaddr *) addr_;
}

socklen_t BufferSock::GetAddressLength() const {
    return addr_len_;
}

ssize_t BufferSock::GetOffset() const {
    return offset_;
}

void BufferSock::UpdateSentDataCount(ssize_t count) {
    offset_ += count;
}

bool BufferSock::IsFinished() const {
    return offset_ >= buffer_->GetContentSize();
}
