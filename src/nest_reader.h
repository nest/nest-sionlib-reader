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
  //TODO: Take measures for two separate lists
  //TODO: Rename parameters rows and values
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
  //TODO: Maybe two buffers

  uint64_t gid;
  uint32_t type;
  std::string name;
  std::string label;
  std::vector<std::string> observables;
  //TODO: Separate lists

  size_t rows;
  size_t values;
  //TODO: Renaming and two values
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
  // TODO: Add getters for sionlib-rec.backend-version and nest version

protected:
  void read_devices(SIONReader&);
  void read_values(SIONReader&, const SIONRankReader::SIONPos&);

private:
  std::map<uint64_t, DeviceData> data;
  //TODO: Split n_val into two
  DeviceData& add_entry(uint64_t dev_id, size_t n_rec, size_t n_val) {
    return data
      .insert(std::make_pair(dev_id, DeviceData(n_rec, n_val)))
      .first->second;
  };

  double t_start;
  double t_end;
  double resolution;

  //TODO: Add attributes for versions
};

#endif // NESTIO_H
