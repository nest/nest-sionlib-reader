#ifndef RAW_MEMORY_H
#define RAW_MEMORY_H

#include <cstring>

class RawMemory
{
private:
    int ptr;
    int max_size;

public:
    char* buffer;

    RawMemory(const RawMemory& other);
    RawMemory(size_t size);
    ~RawMemory();
    void write(const char* v, long unsigned int n);
    int get_capacity();
    int get_size();
    int get_free();
    void clear();
    void* get_ptr();
    template <typename T>
    RawMemory& operator<<(const T data);
};

template <typename T>
RawMemory& RawMemory::operator<<(const T data)
{
    write((const char*) &data, sizeof(T));
    return *this;
}

#endif // RAW_MEMORY_H
