#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#ifndef NESTIO_H
#define NESTIO_H

#include <vector>
#include <string>
#include <map>
#include <utility>

#include "sion.h"

#include "raw_memory.h"
#include "sion_reader.h"


struct DeviceData
{
    DeviceData(std::pair<size_t, size_t> shape)
        : data(shape.first * shape.second * sizeof(double)), shape(shape)
    {
    }

    double* get_data_ptr()
    {
      double* data_ptr = reinterpret_cast<double*>(data.get_ptr());
      return data_ptr;
    }

    int gid;
    std::string name;
    std::string label;
    std::vector<std::string> observables;

    std::pair<size_t, size_t> shape;
    RawMemory data;
};

class Reader
{
public:
    Reader(std::string filename);

    DeviceData* get_device_data(int device_gid);
	std::vector<int> list_devices();

    size_t normal_fread(const void* data, size_t size, size_t nitems, int sid);
    size_t swapped_fread(const void* data, size_t size, size_t nitems, int sid);

private:
    std::map<int, DeviceData> data_;
    size_t (Reader::*freader)(const void* data, size_t size, size_t nitems, int sid);
};

#endif // NESTIO_H
