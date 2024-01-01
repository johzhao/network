#ifndef BUFFER_H
#define BUFFER_H

#include <memory>

class Buffer {
public:
    using Ptr = std::shared_ptr<Buffer>;

    explicit Buffer();

    ~Buffer();

public:
    size_t GetCapacity() const;

    void SetCapacity(size_t capacity);

    char *GetData() const;

    void Assign(const char *data, size_t size = 0);

    size_t GetSize() const;

    void SetSize(size_t size);

private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    char *data_ = nullptr;
};

#endif //BUFFER_H
