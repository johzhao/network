#ifndef BUFFER_H
#define BUFFER_H

#include <memory>

#include "error_code.h"

class Buffer {
public:
    using Ptr = std::shared_ptr<Buffer>;

    explicit Buffer();

    explicit Buffer(const char *data, int size);

    virtual ~Buffer();

public:
    virtual const char *GetData() const;

    virtual int GetContentSize() const;

private:
    const char *buffer_;
    int content_size_;
};

#endif //BUFFER_H
