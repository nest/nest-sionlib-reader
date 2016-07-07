#ifndef NESTIO_H
#define NESTIO_H

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <vector>
#include <string>
#include <map>
#include <utility>

#include "sion.h"

#include "raw_memory.h"
#include "sion_reader.h"


struct DeviceData
{
  DeviceData(size_t rows, size_t values)
    : data(rows * (sizeof(uint64_t)
		   + sizeof(int64_t)
		   + sizeof(double)
		   + values * sizeof(double)))
    , rows(rows)
    , values(values)
  {}

  char* get_data() {return data.buffer;};

  uint64_t gid;
  uint32_t type;
  std::string name;
  std::string label;
  std::vector<std::string> observables;

  RawMemory data;
  size_t rows;
  size_t values;
};

class NestReader
{
public:
  NestReader(const std::string& filename);
  
  DeviceData* get_device_data(int device_gid);
  std::vector<int> list_devices();

  double get_start() {return t_start;};
  double get_end() {return t_end;};
  double get_duration() {return duration;};

protected:
  void read_next_device(SIONReader&);
  void read_next_values(SIONTaskReader&);

private:
  std::map<uint64_t, DeviceData> data;
  DeviceData& add_entry(uint64_t dev_id, size_t n_rec, size_t n_val) {
    return data
      .insert(std::make_pair(dev_id, DeviceData(n_rec, n_val)))
      .first->second;
  };

  double t_start;
  double t_end;
  double duration;
};

#endif // NESTIO_H
