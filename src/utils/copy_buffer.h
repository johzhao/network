#ifndef COPY_BUFFER_H
#define COPY_BUFFER_H

#include <memory>
#include "buffer.h"

class CopyBuffer : public Buffer {
public:
    using Ptr = std::shared_ptr<CopyBuffer>;

    explicit CopyBuffer(const char *data, int size);

    explicit CopyBuffer(Buffer::Ptr &data);

    CopyBuffer(CopyBuffer &other) = delete;

    CopyBuffer operator=(CopyBuffer &other) = delete;

    ~CopyBuffer() override;

public:
    const char *GetData() const override;

    int GetContentSize() const override;

private:
    char *buffer_;
    int size_;
};

#endif //COPY_BUFFER_H
