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

  t_start    = reader.read<double>();
  t_end      = reader.read<double>();
  resolution = reader.read<double>();
  sionlib_rec_backend_version = reader.read<sion_int32>();
  char nest_version_buffer[128];
  reader.read(nest_version_buffer);
  nest_version = nest_version_buffer;

  read_devices(reader);

  //TODO: Use better variable names for info_XXX
  for (int rank = 0; rank < reader.get_ranks(); ++rank)
  {
    if (rank) { // rank > 0, end is just the END_POS
      reader.seek(rank, SION_END_POS);
      reader.get_current_location(&info_blk, &info_pos);
    } // else rank == 0, use beginning of metadata
    read_values(reader, {rank, info_blk, info_pos});
  }
}

void NestReader::read_devices(SIONReader& reader)
{
  auto n_dev = reader.read<sion_uint64>();
  for (size_t i = 0; i < n_dev; ++i) {
    auto dev_id = reader.read<sion_uint64>();
    auto type = reader.read<sion_uint32>();
    
    char name[16];
    char label[16];
    reader.read(name);
    reader.read(label);
    
    auto origin = reader.read<sion_int64>();
    auto t_start = reader.read<sion_int64>();
    auto t_stop = reader.read<sion_int64>();
    auto n_rec = reader.read<sion_uint64>();
    auto double_n_val = reader.read<sion_uint32>();
    auto long_n_val = reader.read<sion_uint32>();

    std::vector<std::string> double_observables;
    for (size_t j = 0; j < double_n_val; ++j)
    {
      char ob_name[8];
      reader.read(ob_name);
      double_observables.push_back(ob_name);
    }
    std::vector<std::string> long_observables;
    for (size_t j = 0; j < long_n_val; ++j)
    {
      char ob_name[8];
      reader.read(ob_name);
      long_observables.push_back(ob_name);
    }

    DeviceData& entry = add_entry(dev_id, n_rec, double_n_val, long_n_val);

    entry.gid = dev_id;
    entry.type = type;
    entry.name = name;
    entry.label = label;
    entry.origin = origin;
    entry.t_start = t_start;
    entry.t_stop = t_stop;
    entry.double_observables = double_observables;
    entry.long_observables = long_observables;
  }
}

void NestReader::read_values(SIONReader& reader, const SIONRankReader::SIONPos& v)
{
  auto rank_ptr = reader.make_rank_reader(v);

  while (! rank_ptr->eof()) {
    auto device_gid   = rank_ptr->read<sion_uint64>();
    auto neuron_gid   = rank_ptr->read<sion_uint64>();
    auto step         = rank_ptr->read<sion_int64>();
    auto offset       = rank_ptr->read<double>();
    auto double_n_val = rank_ptr->read<sion_uint32>();
    auto long_n_val   = rank_ptr->read<sion_uint32>();

    RawMemory& buffer = *data.find(device_gid)->second.raw;
    buffer << neuron_gid << step << offset;

    auto double_subbuf = buffer.get_region<double>(double_n_val);
    rank_ptr->read(double_subbuf, double_n_val);
    auto long_subbuf = buffer.get_region<long>(long_n_val);
    rank_ptr->read(long_subbuf, long_n_val);
  }
}

DeviceData& NestReader::get_device_data(uint64_t device_gid)
{
    auto tmp = data.find(device_gid);
    if (tmp == data.end()) {
      std::stringstream msg;
      msg << "unknown device gid #" << device_gid;
      throw std::out_of_range(msg.str());
    }
    return tmp->second;
}

std::vector<uint64_t> NestReader::list_devices()
{
  std::vector<uint64_t> gids;
  for (auto&& e: data)
    gids.push_back(e.second.gid);
  
  return gids;
}
