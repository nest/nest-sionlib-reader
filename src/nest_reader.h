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
#include <memory>

#include "sion.h"

#include "raw_memory.h"
#include "sion_reader.h"


struct DeviceData
{
  DeviceData(size_t rows, size_t values)
    : raw(std::make_shared<RawMemory>
	  (rows * (sizeof(uint64_t)
		   + sizeof(int64_t)
		   + sizeof(double)
		   + values * sizeof(double)))
	  )
    , rows(rows)
    , values(values)
  {};

  char* get_raw() {return raw->get_buffer();};

  std::shared_ptr<RawMemory> raw;

  uint64_t gid;
  uint32_t type;
  std::string name;
  std::string label;
  std::vector<std::string> observables;

  size_t rows;
  size_t values;
};

class NestReader
{
public:
  NestReader(const std::string& filename);
  
  DeviceData& get_device_data(uint64_t device_gid);
  DeviceData* get_device_data_ptr(uint64_t device_gid) {
    return &get_device_data(device_gid);
  };
  std::vector<uint64_t> list_devices();

  double get_start() {return t_start;};
  double get_end() {return t_end;};
  double get_resolution() {return resolution;};

protected:
  void read_devices(SIONReader&);
  void read_values(SIONReader&, const SIONRankReader::SIONPos&);

private:
  std::map<uint64_t, DeviceData> data;
  DeviceData& add_entry(uint64_t dev_id, size_t n_rec, size_t n_val) {
    return data
      .insert(std::make_pair(dev_id, DeviceData(n_rec, n_val)))
      .first->second;
  };

  double t_start;
  double t_end;
  double resolution;
};

#endif // NESTIO_H
