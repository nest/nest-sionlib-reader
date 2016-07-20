#ifndef SION_READER_H
#define SION_READER_H

#include <cstring>
#include <type_traits>
#include <utility>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include "sion.h"

class SIONEndian {
 public:
  SIONEndian(bool do_swap): do_swap(do_swap) {};
  
  template<typename T>
  using is_pointer = typename std::enable_if<std::is_pointer<T>::value>::type;

  template<typename T, typename = is_pointer<T> >
  void swap(T buf, size_t nitems = 1) {
    if (do_swap) swap_array(buf, nitems);
  }

  template<typename T, size_t nitems>
  void swap(T (&buf)[nitems]) {
    if (do_swap) swap_array(buf, nitems);
  }

  void swap(char* buf, size_t nitems = 1) {};

protected:
  template<typename T>
  static void swap_elem(T* subbuf) {
    char* start = reinterpret_cast<char*>(subbuf);
    char* end   = start + sizeof(T) - 1;
    char* mid   = start + sizeof(T) / 2; 
    for (; end >= mid; start++, end--)
      std::swap(*start, *end);
  };

  template<typename T>
  static void swap_array(T* buf, size_t nitems) {
    for (T* i = buf; i < buf+nitems; i++)
      swap_elem(i);
  };
  
private:
  const bool do_swap;
};

class SIONRankReader;

class SIONReader
{
public:
  SIONReader(const std::string& filename);
  ~SIONReader();
  
  void seek(int rank, sion_int64 blk = 0, sion_int64 pos = 0);
  sion_int64 get_size(int rank, sion_int64 blk) {
    return blk_sizes[n_ranks * blk + rank];
  }
  void get_current_location(sion_int64* blk, sion_int64* pos);
  int get_ranks() {return n_ranks;};

  // aggregate params for SIONRankReader()
  struct SIONPos {
    int rank;
    sion_int64 blk;
    sion_int64 pos;
  };

  // SIONRankReader factory
  inline
  std::unique_ptr<SIONRankReader>
  make_rank_reader(const SIONPos&);

  template<typename T>
  using is_pointer = typename std::enable_if<std::is_pointer<T>::value>::type;

  template<typename T, typename = is_pointer<T> >
  void read(T data, size_t nitems = 1) {
    fread(reinterpret_cast<char*>(data),
	  sizeof(data[0]),
	  nitems);
    swapper.swap(data, nitems);
  };

  template<typename T, size_t nitems>
  void read(T (&data)[nitems]) {
    return read(data, nitems);
  };

  template<typename T>
  T read() {
    T r;
    read(&r);
    return r;
  };

  template<typename T>
  void swap(T buf, size_t nitems) {
    swapper.swap(buf, nitems);
  };

  template<typename T>
  void swap(T buf) {
    swapper.swap(buf);
  };

protected:
  struct SIONFile {
    int sid; int n_ranks; sion_int64* blk_sizes;
    SIONFile(const std::string& filename);
  };
  SIONReader(const SIONFile&);

  void fread(char* data, size_t size, size_t nitems);

private:
  int sid;
  int n_ranks;
  sion_int64* blk_sizes;
  SIONEndian swapper;
};

class SIONRankReader {
public:
  using SIONPos = SIONReader::SIONPos;

  SIONRankReader(SIONReader*, const SIONPos&);

  bool eof() const {
    return blk-1 == eof_blk
      && pos-buffer.begin() == eof_pos;
  }

  template<typename T>
  void read(T data, size_t nitems = 1) {
    readbuf(data, nitems);
    reader->swap(data, nitems);
  }

  template<typename T>
  T read() {T r; read(&r); return r;};

protected:
  template <typename T>
  void readbuf(T* data, size_t nitems) {
    readbuf(reinterpret_cast<char*>(data),
	    nitems*sizeof(T));
  };

  // base not-templated function
  void readbuf(char* data, size_t size);
  
private:
  void fetch_chunk();

  SIONReader* reader;
  
  sion_int64 rank;
  sion_int64 blk;

  std::vector<char> buffer;
  std::vector<char>::iterator pos;

  sion_int64 eof_blk;
  sion_int64 eof_pos;
};

std::unique_ptr<SIONRankReader>
SIONReader::make_rank_reader(const SIONPos& v) {
  return std::unique_ptr<SIONRankReader>(new SIONRankReader(this, v));
};

#endif // SION_READER_H
