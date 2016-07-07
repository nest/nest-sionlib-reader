#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cstddef>

#include <type_traits>

#include "nest_reader.h"

NestReader::NestReader(const std::string& filename)
{
  SIONReader reader(filename);

  // get tail
  auto tail_size = 2 * sizeof(sion_int64);
  // Could we split at zero??? Check
  reader.seek(0, SION_END_POS, -tail_size);

  // read tail for task 0
  auto info_blk = reader.read<sion_int64>();
  auto info_pos = reader.read<sion_int64>();

  // read info section for task 0
  reader.seek(0, info_blk, info_pos);

  t_start  = reader.read<double>();
  t_end    = reader.read<double>();
  duration = reader.read<double>();

  auto n_dev = reader.read<sion_uint64>();
  for (size_t i = 0; i < n_dev; ++i)
    read_next_device(reader);

  for (int task = 0; task < reader.get_tasks(); ++task)
  {
    if (task) { // task > 0, end is just the END_POS
      reader.seek(task, SION_END_POS);
      reader.get_current_location(&info_blk, &info_pos);
    }
    SIONTaskReader treader(reader, task, info_blk, info_pos);
    while (! treader.eof())
      read_next_values(treader);
  }
}

void NestReader::read_next_device(SIONReader& reader)
{
  auto dev_id = reader.read<sion_uint64>();
  auto type = reader.read<sion_uint32>();
    
  char name[16];
  char label[16];
  reader.read(name);
  reader.read(label);
    
  auto n_rec = reader.read<sion_uint64>();
  auto n_val = reader.read<sion_uint32>();
    
  std::vector<std::string> observables;
  for (size_t j = 0; j < n_val; ++j)
  {
    char ob_name[8];
    reader.read(ob_name);
    observables.push_back(ob_name);
  }

  DeviceData& entry = add_entry(dev_id, n_rec, n_val);
  
  entry.gid = dev_id;
  entry.type = type;
  entry.name = name;
  entry.label = label;
  entry.observables = observables;
}

void NestReader::read_next_values(SIONTaskReader& treader) {
  auto device_gid = treader.read<sion_uint64>();
  auto neuron_gid = treader.read<sion_uint64>();
  auto step       = treader.read<sion_int64>();
  auto offset     = treader.read<double>();
  auto n_values   = treader.read<sion_uint32>();

  RawMemory& buffer = data.find(device_gid)->second.data;
  buffer << neuron_gid << step << offset;

  auto subbuf = buffer.get_region<double>(n_values);
  treader.read(subbuf, n_values);
}

DeviceData* NestReader::get_device_data(uint64_t device_gid)
{
    auto tmp = data.find(device_gid);
    if(tmp == data.end())
    {
      std::stringstream msg;
      msg << "unknown device gid #" << device_gid;
      throw std::out_of_range(msg.str());
    }
    return &tmp->second;
}

std::vector<uint64_t> NestReader::list_devices()
{
  std::vector<uint64_t> gids;
  for (auto&& e: data)
    gids.push_back(e.second.gid);
  
  return gids;
}
