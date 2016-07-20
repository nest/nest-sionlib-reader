#include "sion_reader.h"

#include <stdexcept>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>

class sion_error: public std::runtime_error {
public:
  sion_error(const std::string& what_arg )
    : std::runtime_error(what_arg)
  {};
};

void SIONReader::get_current_location(sion_int64* blk, sion_int64* pos) {
  int lblk; // interface mismatch in sionlib api
  int maxchunks;
  sion_int64* chunksizes = nullptr;
  
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
  
  int max_chunks;
  sion_int64 global_skip;
  sion_int64 start_of_varheader;
  sion_int64 *chunk_sizes;
  sion_int64 *global_ranks;
  sion_int64 *block_count;
		       
  sid = sion_open((char*) filename.c_str(),
		  "rb",
		  &n_ranks,
		  &n_files,
		  nullptr,
		  &fs_block_size,
		  nullptr,
		  nullptr);
  if (sid == -1)
    throw sion_error(std::string("sion_open: ") + filename);

  // set n_ranks, blk_sizes
  if (sion_get_locations(sid,
			 &n_ranks,
			 &max_chunks,
			 &global_skip,
			 &start_of_varheader,
			 &chunk_sizes,
			 &global_ranks,
			 &block_count,
			 &blk_sizes)
      != SION_SUCCESS)
    throw sion_error(std::string("sion_get_locations: ") + filename);
}

SIONReader::SIONReader(const std::string& filename)
  : SIONReader(SIONFile(filename))
{}

SIONReader::SIONReader(const SIONFile& file)
  : sid(file.sid)
  , n_ranks(file.n_ranks)
  , blk_sizes(file.blk_sizes)
  , swapper(sion_endianness_swap_needed(sid))
{};

SIONReader::~SIONReader() {
  if (sion_close(sid) != SION_SUCCESS)
    throw sion_error("sion_close");
}

void SIONReader::seek(int rank, sion_int64 blk, sion_int64 pos) {
  if (sion_seek(sid, rank, blk, pos) != SION_SUCCESS) {
    std::stringstream msg;
    msg << "sion_seek:"
	<< " rank=" << rank
	<< " blk=" << blk
	<< " pos=" << pos;
    throw sion_error(msg.str());
  }
}

void SIONReader::fread(char* data, size_t size, size_t nitems) {
  size_t r = sion_fread(data, size, nitems, sid);
  if (r < nitems) {
    std::stringstream msg;
    msg << "sion_fread:"
	<< " r=" << r
	<< " size=" << size
	<< " nitems=" << nitems;
    throw sion_error(msg.str());
  }
}

SIONRankReader::SIONRankReader(SIONReader* reader, const
			       SIONPos& v)
  : reader(reader)
  , rank(v.rank)
  , blk(0)
  , buffer(0)
  , pos(nullptr)
  , eof_blk(v.blk)
  , eof_pos(v.pos)
{
  fetch_chunk();
}

void SIONRankReader::fetch_chunk()
{
  if (blk > eof_blk) {
    std::stringstream msg;
    msg << "fetch_chunk overflow: "
	<< "blk=" << blk << " "
	<< "eof_blk=" << eof_blk;
    throw std::out_of_range(msg.str());
  }
  reader->seek(rank, blk, 0);
  
  size_t chunk_size = (blk == eof_blk)
    ? eof_pos : reader->get_size(rank, blk);
  buffer.resize(chunk_size);
  
  pos = buffer.begin();
  reader->read(&buffer[0], chunk_size);
  blk += 1;
}

void SIONRankReader::readbuf(char* data, size_t size)
{
  while (size)
  {
    size_t len = buffer.end()-pos;
    size_t available = std::min(size, len);
    size -= available;

    auto next_pos = pos + available;
    std::copy(pos, next_pos, data);
    pos = next_pos;
    data += available;

    if (size) fetch_chunk();
  }
}
