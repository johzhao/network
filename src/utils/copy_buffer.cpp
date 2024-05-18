#include "copy_buffer.h"

#include <cstring>

CopyBuffer::CopyBuffer(const char *data, int size)
        : buffer_(new char[size + 1]), size_(size) {
    memset(buffer_, 0, size_ + 1);
    memcpy(buffer_, data, size_);
}

CopyBuffer::CopyBuffer(Buffer::Ptr &data)
        : CopyBuffer(data->GetData(), data->GetContentSize()) {
}

CopyBuffer::~CopyBuffer() {
    delete[] buffer_;
}

const char *CopyBuffer::GetData() const {
    return buffer_;
}

int CopyBuffer::GetContentSize() const {
    return size_;
}
