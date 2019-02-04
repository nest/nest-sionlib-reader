#ifndef SION_READER_H
#define SION_READER_H

#include <algorithm>
#include <cstring>

#include "sion.h"

class SIONReader
{
public:
    SIONReader(int sid);
    ~SIONReader();
    
	void seek(int task, int chunk, int pos);
    sion_int64 get_position();
    sion_int64 get_size();
    sion_int64 get_chunk();
    
	template <typename T>
    void read(T* buffer, int count);

private:
    void fetch_chunk_(int chunk);

    int sid_;

    sion_int64 task_;
    sion_int64 chunk_;
    sion_int64 size_;
    sion_int64 position_;

    char* buffer_;
    size_t buffer_size_;
};

template <typename T>
void SIONReader::read(T* buffer, int count)
{
    sion_int64 required_size = count * sizeof(T);

    size_t copied = std::min(required_size, size_ - position_);
    size_t remaining = required_size - copied;
    std::memcpy(buffer, buffer_ + position_, copied);

    position_ += copied;

    if (remaining > 0)
    {
        fetch_chunk_(chunk_ + 1);
        std::memcpy(buffer + copied, buffer_ + position_, remaining);
        position_ += remaining;
    }
}

#endif // SION_READER_H
