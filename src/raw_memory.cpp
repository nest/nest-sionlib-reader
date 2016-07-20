#include <iostream>

#include "raw_memory.h"

RawMemory::RawMemory(size_t size)
  : sbuf(size)
  , buffer(&sbuf[0])
  , start(buffer)
  , end(buffer+size)
{}

void RawMemory::write(const char* v, size_t n)
{
  auto subbuf = get_region<char>(n);
  memcpy(subbuf, v, n);
}
