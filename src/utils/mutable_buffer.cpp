#include "mutable_buffer.h"

#include <cstring>

MutableBuffer::MutableBuffer(int capacity) {
    capacity_ = capacity;
    buffer_ = new char[capacity_];
    content_size_ = 0;
}

MutableBuffer::~MutableBuffer() {
    delete[] buffer_;
}

int MutableBuffer::GetCapacity() const {
    return capacity_;
}

ErrorCode MutableBuffer::SetCapacity(int capacity) {
    if (capacity < content_size_) {
        return Buffer_Not_Enough_Capacity;
    }

    do {
        if (capacity > capacity_) {
            //请求的内存大于当前内存，那么重新分配
            break;
        }

        if (capacity_ < 2 * 1024) {
            //2K以下，不重复开辟内存，直接复用
            return Success;
        }

        if (2 * capacity > capacity_) {
            //如果请求的内存大于当前内存的一半，那么也复用
            return Success;
        }
    } while (false);

    auto buffer = new char [capacity];
    memcpy(buffer, buffer_, content_size_);

    delete[] buffer_;
    buffer_ = buffer;
    capacity_ = capacity;

    return Success;
}

const char *MutableBuffer::GetData() const {
    return buffer_;
}

char *MutableBuffer::GetWritableData() const {
    return buffer_ + GetContentSize();
}

int MutableBuffer::GetContentSize() const {
    return content_size_;
}

int MutableBuffer::GetAvailableSpace() const {
    return capacity_ - content_size_;
}

ErrorCode MutableBuffer::AppendData(const char *data, int length) {
    if (GetAvailableSpace() < length) {
        return Buffer_Not_Enough_Capacity;
    }

    memcpy(buffer_ + content_size_, data, length);
    content_size_ += length;

    return Success;
}

ErrorCode MutableBuffer::IncreaseContentSize(int increased_size) {
    if (GetAvailableSpace() < increased_size) {
        return Buffer_Not_Enough_Capacity;
    }

    content_size_ += increased_size;

    return Success;
}

void MutableBuffer::ConsumeData(int length) {
    if (length >= content_size_) {
        Reset();
    } else {
        memmove(buffer_, buffer_ + length, content_size_ - length);
        content_size_ -= length;
    }
}

void MutableBuffer::Reset() {
    content_size_ = 0;
}
