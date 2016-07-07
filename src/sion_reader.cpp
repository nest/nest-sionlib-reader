#include "sion_reader.h"

#include <stdexcept>
#include <string>
#include <sstream>
#include <algorithm>

class sion_error: public std::runtime_error {
public:
  sion_error(const std::string& what_arg )
    : std::runtime_error(what_arg)
  {};
};

void SIONReader::get_current_location(sion_int64* blk, sion_int64* pos) {
  int lblk; // interface mismatch in sionlib api
  int maxchunks;
  sion_int64* chunksizes;
  
  if (sion_get_current_location(sid, &lblk, pos, &maxchunks, &chunksizes)
      != SION_SUCCESS)
    throw sion_error("sion_get_current_location");
  *blk = lblk;
}

SIONReader::SIONFile::SIONFile(const std::string& filename)
{  
  // throw aways for interface
  int n_files;
  sion_int32 fs_block_size;
  int* ranks;
  FILE* fh;

  sid = sion_open((char*) filename.c_str(),
		  "rb",
		  &n_tasks,
		  &n_files,
		  &chunk_sizes,
		  &fs_block_size,
		  &ranks,
		  &fh);
  if (sid == -1)
    throw sion_error(std::string("sion_open: ") + filename);
}

SIONReader::SIONReader(const std::string& filename)
  : SIONReader(SIONFile(filename))
{}

SIONReader::SIONReader(const SIONFile& file)
  : sid(file.sid)
  , n_tasks(file.n_tasks)
  , chunk_sizes(file.chunk_sizes)
  , swapper(sion_endianness_swap_needed(sid))
{};

SIONReader::~SIONReader() {
  if (sion_close(sid) != SION_SUCCESS)
    throw sion_error("sion_close");
}

void SIONReader::seek(int rank, size_t chunk, size_t pos) {
  if (sion_seek(sid, rank, chunk, pos) != SION_SUCCESS) {
    std::stringstream msg;
    msg << "sion_seek:"
	<< " rank=" << rank
	<< " chunk=" << chunk
	<< " pos=" << pos;
    throw sion_error(msg.str());
  }
}

SIONTaskReader::SIONTaskReader(SIONReader& reader,
			       int task,
			       sion_int64 eof_chunk,
			       sion_int64 eof_pos)
  : reader(reader)
  , task(task)
  , chunk(0)
  , chunk_size(0)
  , buffer(NULL)
  , buffer_size(0)
  , start(NULL)
  , end(NULL)
  , eof_chunk(eof_chunk)
  , eof_pos(eof_pos)
{
  fetch_chunk(0);
}

SIONTaskReader::~SIONTaskReader()
{
  delete [] buffer;
}

void SIONTaskReader::fetch_chunk(int chunk_)
{
  chunk = chunk_;
  reader.seek(task, chunk, 0);
  chunk_size = reader.get_size(task, chunk);
  
  if (static_cast<size_t>(chunk_size) > buffer_size) {
    delete [] buffer;
    buffer = new char[chunk_size];
    buffer_size = chunk_size;
  }
  
  start = buffer;
  end = buffer + chunk_size;
  reader.read(buffer, chunk_size);
}

void SIONTaskReader::readbuf(char* data, size_t size) {
  size_t copied = std::min(size, static_cast<size_t>(end-start));
  size_t remaining = size - copied;

  std::memcpy(data, start, copied);
  start += copied;
  
  if (remaining > 0)
  {
    fetch_chunk(chunk + 1);
    std::memcpy(data + copied, start, remaining);
    start += remaining;
  }
}
