#ifndef SION_READER_H
#define SION_READER_H

#include <cstring>
#include <type_traits>
#include <utility>
#include <string>

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

class SIONReader
{
public:
  SIONReader(const std::string& filename);
  ~SIONReader();
  
  void seek(int rank, size_t chunk = 0, size_t pos = 0);
  sion_int64 get_size(int task, size_t chunk) {
    return chunk_sizes[n_tasks * chunk + task];
  }
  void get_current_location(sion_int64* blk, sion_int64* pos);
  int get_tasks() {return n_tasks;};

  template<typename T>
  size_t read(T data, size_t nitems = 1) {
    size_t r = sion_fread(data, sizeof(T[0]), nitems, sid);
    swapper.swap(data, nitems);
    return r;
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
    int sid; int n_tasks; sion_int64* chunk_sizes;
    SIONFile(const std::string& filename);
  };
  SIONReader(const SIONFile&);

private:
  int sid;
  int n_tasks;
  sion_int64* chunk_sizes;
  SIONEndian swapper;
};

class SIONTaskReader {
public:
  SIONTaskReader(SIONReader&, int task, sion_int64 eof_chunk, sion_int64 eof_pos);
  ~SIONTaskReader();
  
  sion_int64 get_size() {return chunk_size;};
  sion_int64 get_chunk() {return chunk;};

  bool eof() const {
    return chunk > eof_chunk
      || (chunk == eof_chunk && end-start > eof_pos);
  }

  template<typename T>
  void read(T data, size_t nitems = 1) {
    readbuf(data, nitems);
    reader.swap(data, nitems);
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
  void fetch_chunk(int chunk);

  SIONReader reader;
  
  sion_int64 task;
  sion_int64 chunk;
  sion_int64 chunk_size;

  char* buffer;
  size_t buffer_size;
  char* start;
  char* end;

  sion_int64 eof_chunk;
  sion_int64 eof_pos;
};

#endif // SION_READER_H
