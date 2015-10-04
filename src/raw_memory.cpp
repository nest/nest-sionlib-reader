#include <iostream>

#include "raw_memory.h"

RawMemory::RawMemory(size_t size) : ptr(0)
{
    if (size > 0)
    {
        buffer = new char[size];
        max_size = size;
    }
}

RawMemory::RawMemory(const RawMemory& other)
{
    buffer = new char[other.max_size];
    max_size = other.max_size;
    ptr = other.ptr;
}

RawMemory::~RawMemory()
{
    if (buffer != NULL)
        delete[] buffer;
}

void RawMemory::write(const char* v, long unsigned int n)
{
    if (ptr + n <= max_size)
    {
        memcpy(buffer + ptr, v, n);
        ptr += n;
    }
    else
    {
        std::cerr << "RawMemory: buffer overflow: ptr=" << ptr << " n=" << n
                  << " max_size=" << max_size << std::endl;
    }
}

int RawMemory::get_size()
{
    return ptr;
}

int RawMemory::get_capacity()
{
    return max_size;
}

int RawMemory::get_free()
{
    return (max_size - ptr);
}

void RawMemory::clear()
{
    ptr = 0;
}

void* RawMemory::get_ptr()
{
    return (void*) buffer;
}
