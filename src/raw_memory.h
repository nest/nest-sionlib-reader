#ifndef RAW_MEMORY_H
#define RAW_MEMORY_H

#include <cstring>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

class RawMemory
{
private:
  std::vector<char> sbuf;

  char* const buffer;
  char* start;
  char* const end;

public:
  RawMemory(size_t size);
  RawMemory(const RawMemory&) = delete;
  
  void write(const char* v, size_t n = 1);
  
  template<typename T>
  RawMemory& operator<<(const T& data);

  template<typename T>
  T* get_region(size_t n);

  char* get_buffer() {return buffer;};
};

template<typename T>
RawMemory& RawMemory::operator<<(const T& data)
{
  write(reinterpret_cast<const char*>(&data), sizeof(T));
  return *this;
}

template<typename T>
T* RawMemory::get_region(size_t n) {
  char* next = start + n*sizeof(T);
  if (next > end)
  {
    std::stringstream msg;
    msg << "RawMemory: buffer overflow: start=" << start
	<< " n=" << n*sizeof(T)
	<< " end=" << end
	<< std::endl;
    throw std::out_of_range(msg.str());
  }

  char* last = start;
  start = next;
  return reinterpret_cast<T*>(last);
}

#endif // RAW_MEMORY_H
