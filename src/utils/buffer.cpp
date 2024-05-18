#include "buffer.h"

Buffer::Buffer()
        : buffer_(nullptr), content_size_(0) {
}

Buffer::Buffer(const char *data, int size)
        : buffer_(data), content_size_(size) {
}

Buffer::~Buffer() = default;

const char *Buffer::GetData() const {
    return buffer_;
}

int Buffer::GetContentSize() const {
    return content_size_;
}
