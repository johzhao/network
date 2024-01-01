#include "buffer.h"

#include <cstring>

#include "loguru/loguru.hpp"

Buffer::Buffer() {
}

Buffer::~Buffer() {
    delete[] data_;
}

size_t Buffer::GetCapacity() const {
    return capacity_;
}

void Buffer::SetCapacity(size_t capacity) {
    if (data_) {
        do {
            if (capacity > capacity_) {
                //请求的内存大于当前内存，那么重新分配
                break;
            }

            if (capacity_ < 2 * 1024) {
                //2K以下，不重复开辟内存，直接复用
                return;
            }

            if (2 * capacity > capacity_) {
                //如果请求的内存大于当前内存的一半，那么也复用
                return;
            }
        } while (false);

        delete[] data_;
    }

    data_ = new char[capacity + 1];
    capacity_ = capacity;
}

char *Buffer::GetData() const {
    return data_;
}

void Buffer::Assign(const char *data, size_t size) {
    if (size == 0) {
        size = strlen(data);
    }

    SetCapacity(size + 1);
    memcpy(data_, data, size);
    data_[size] = '\0';
    SetSize(size);
}

size_t Buffer::GetSize() const {
    return size_;
}

void Buffer::SetSize(size_t size) {
    if (size > GetCapacity()) {
        LOG_F(ERROR, "Buffer::SetSize out of range");
        throw std::invalid_argument("Buffer::SetSize out of range");
    }

    size_ = size;
}
