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
DeviceData(size_t rows, size_t double_n_val, size_t long_n_val)
    : raw(std::make_shared<RawMemory>
	  (rows * (sizeof(uint64_t)
		   + sizeof(int64_t)
		   + sizeof(double)
		   + double_n_val * sizeof(double)
                   + long_n_val * sizeof(long)))
	  )
    , rows(rows)
    , double_n_val(double_n_val)
    , long_n_val(long_n_val)
  {};

  char* get_raw() {return raw->get_buffer();};

  std::shared_ptr<RawMemory> raw;

  uint64_t gid;
  uint32_t type;
  std::string name;
  std::string label;
  long origin;
  long t_start;
  long t_stop;
  std::vector<std::string> double_observables;
  std::vector<std::string> long_observables;

  size_t rows; // number of data points
  size_t double_n_val;
  size_t long_n_val;
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

  double get_start() const { return t_start; };
  double get_end() const { return t_end; };
  double get_resolution() const { return resolution; };
  int get_sionlib_rec_backend_version() const { return sionlib_rec_backend_version; };
  std::string get_nest_version() const { return nest_version; };

protected:
  void read_devices(SIONReader&);
  void read_values(SIONReader&, const SIONRankReader::SIONPos&);

private:
  std::map<uint64_t, DeviceData> data;
  DeviceData& add_entry( uint64_t dev_id, size_t n_rec, size_t double_n_val, size_t long_n_val ) {
    return data
      .insert(std::make_pair(dev_id, DeviceData(n_rec, double_n_val, long_n_val)))
      .first->second;
  };

  double t_start;
  double t_end;
  double resolution;
  int sionlib_rec_backend_version;
  std::string nest_version;
};

#endif // NESTIO_H
