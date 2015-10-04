#include "sion_reader.h"

SIONReader::SIONReader(int sid)
    : sid_(sid), task_(0), chunk_(0), position_(0), size_(0), buffer_size_(0), buffer_(NULL)
{
}

SIONReader::~SIONReader()
{
    if (buffer_ != NULL)
        delete[] buffer_;
}

void SIONReader::seek(int task, int chunk, int pos)
{
    task_ = task;
    fetch_chunk_(chunk);

    chunk_ = chunk;
    position_ = pos;
}

sion_int64 SIONReader::get_position()
{
    return position_;
}

sion_int64 SIONReader::get_size()
{
    return size_;
}

sion_int64 SIONReader::get_chunk()
{
    return chunk_;
}

void SIONReader::fetch_chunk_(int chunk)
{
    if (buffer_ != NULL)
        delete[] buffer_;

    int ntasks;
    int maxblocks;
    sion_int64 globalskip;
    sion_int64 start_of_varheader;
    sion_int64* localsizes;
    sion_int64* globalranks;
    sion_int64* chunkcounts;
    sion_int64* chunksizes;

    sion_get_locations(sid_,
        &ntasks,
        &maxblocks,
        &globalskip,
        &start_of_varheader,
        &localsizes,
        &globalranks,
        &chunkcounts,
        &chunksizes);

    sion_seek(sid_, task_, chunk, 0);
    // size_ = sion_bytes_avail_in_block( sid_ );
    size_ = chunksizes[ntasks * chunk + task_];

    chunk_ = chunk;
    buffer_ = new char[size_]();
    position_ = 0;

    sion_fread(buffer_, 1, size_, sid_);
}
