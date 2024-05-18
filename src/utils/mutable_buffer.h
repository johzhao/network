#ifndef MUTABLE_BUFFER_H
#define MUTABLE_BUFFER_H

#include <memory>

#include "error_code.h"
#include "buffer.h"

class MutableBuffer : public Buffer {
public:
    using Ptr = std::shared_ptr<MutableBuffer>;

    explicit MutableBuffer(int capacity);

    MutableBuffer(const MutableBuffer &other) = delete;

    MutableBuffer operator=(const MutableBuffer &other) = delete;

    ~MutableBuffer() override;

public:
    int GetCapacity() const;

    ErrorCode SetCapacity(int capacity);

    const char *GetData() const override;

    char *GetWritableData() const;

    int GetContentSize() const override;

    int GetAvailableSpace() const;

    ErrorCode AppendData(const char *data, int length);

    ErrorCode IncreaseContentSize(int increased_size);

    void ConsumeData(int length);

    void Reset();

private:
    int capacity_;
    char *buffer_;
    int content_size_;
};


#endif //MUTABLE_BUFFER_H
