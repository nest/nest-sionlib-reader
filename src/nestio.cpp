#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cstddef>

#include "nestio.h"

class Sion {
public:
  int n_tasks;
  int n_files;
  sion_int64* chunk_sizes;
  sion_int32 fs_block_size;
  int* ranks;
  FILE* fh;

  const int sid;
  const bool do_swap;
  
  enum {LITTLE_ENDIAN = 0, BIG_ENDIAN = 1} sion_endian;

  static const int16_t z1 = 1;
  static const sion_endian my_endian
    = ((static_cast<char*>(&z1))[0] == 0)
      ? BIG_ENDIAN : LITTLE_ENDIAN;

  int getsid(const std::string& filename) {
    return sion_open((char*) filename.c_str(),
		     "rb",
		     &n_tasks,
		     &n_files,
		     &chunk_sizes,
		     &fs_block_size,
		     &ranks,
		     &fh);
  }

  Sion(const std::string& filename)
    : n_tasks(0)
    , n_files(0)
    , chunk_sizes(NULL)
    , fs_block_size(0)
    , ranks(NULL)
    , fh(NULL)
    , sid(getsid(filename))
    , do_swap(sion_get_file_endianness(psid) != my_endian)
  {};

  template<typename T>
  static void swap_elem(T* subbuf) {
    char* start = static_cast<char*>(subbuf);
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

  template<typename T>
  size_t fread(T* data, size_t nitems = 1) {
    size_t r = sion_fread(data, sizeof(T), nitems, sid);
    if (do_swap) swap_array(data, nitems);
    return r;
  }

  size_t fread(char* data, size_t nitems = 1) {
    return sion_fread(data, sizeof(char), nitems, sid);
  }

  void get_locations(int* ntasks,
		     int* maxblocks,
		     sion_int64* globalskip,
		     sion_int64* start_of_varheader,
		     sion_int64** localsizes,
		     sion_int64** globalranks,
		     sion_int64** chunkcounts,
		     sion_int64** chunksizes) {
    sion_get_locations(
	sid,
        ntasks,
        maxblocks,
        globalskip,
        start_of_varheader,
        localsizes,
        globalranks,
        chunkcounts,
        chunksizes
    );
  };

  void seek(int task, size_t chunk, size_t pos = 0) {
    sion_seek(sid, task, chunk, pos);
  };

  sion_int64 bytes_avail_in_block() {
    return sion_bytes_avail_in_block(sid);
  }
};

class Chunker {
public:
  Sion& sion;

  Chunker(Sion& psion): sion(psion) {};
}

Reader::Reader(std::string filename)
{
  Sion sion(filename);

  // find tail
  int ntasks;
  int maxblocks;
  sion_int64 globalskip;
  sion_int64 start_of_varheader;
  sion_int64* localsizes;
  sion_int64* globalranks;
  sion_int64* chunkcounts;
  sion_int64* chunksizes;
  
  sion.get_locations(
        &ntasks,
        &maxblocks,
        &globalskip,
        &start_of_varheader,
        &localsizes,
        &globalranks,
        &chunkcounts,
        &chunksizes
    );

    int task = 0;
    size_t last_chunk = chunkcounts[task] - 1;

    sion.seek(task, last_chunk);
    sion_int64 end = sion.bytes_avail_in_block();
    size_t tail_size = 4 * sizeof(sion_int64) + 3 * sizeof(double);

    // check if tail section was splitted over two chunks and fix seeking
    if (tail_size > end)
    {
        last_chunk--;
        tail_size -= end;
        sion.seek(task, last_chunk);
        end = sion.bytes_avail_in_block(sid);
    }
    sion.seek(task, last_chunk, end - tail_size);

    // read tail
    sion_int64 body_blk, info_blk, body_pos, info_pos;
    sion.fread(&body_blk);
    sion.fread(&body_pos);
    sion.fread(&info_blk);
    sion.fread(&info_pos);

    double t_start, t_end, duration;
    sion.fread(&t_start);
    sion.fread(&t_end);
    sion.fread(&duration);

    // read info section
    sion.seek(task, info_blk, info_pos);

    sion_uint64 n_dev;
    sion.fread(&n_dev);

    for (size_t i = 0; i < n_dev; ++i)
    {
        sion_uint64 dev_id;
	sion_uint32 type;
        char name[16];
        char label[16];
        sion.fread(&dev_id);
        sion.fread(&type);
        sion.fread(name, 16);
        sion.fread(label, 16);

        sion_uint64 n_rec;
        sion.fread(&n_rec);

        sion_uint32 n_val;
        sion.fread(&n_val);

	std::vector<std::string> observables;
        for (size_t j = 0; j < n_val; ++j)
        {
            char ob_name[8];
            sion.fread(ob_name, 8);
            observables.push_back(ob_name);
        }

        size_t entry_size
	  = sizeof(double) + sizeof(double) + n_val * sizeof(double);
        std::pair<size_t, size_t> shape
	  = std::make_pair(n_rec, n_val + 3);

        DeviceData& entry = data_
	  .insert(std::make_pair(dev_id, DeviceData(shape)))
	  .first->second;
        entry.gid = dev_id;
        entry.name = name;
        entry.label = label;
        entry.observables = observables;
    }

    for (int task = 0; task < ntasks; ++task)
    {
        sion_int64 body_blk, info_blk, body_pos, info_pos;

        if (task == 0)
        {
            size_t last_chunk = chunkcounts[task] - 1;
            sion.seek(task, last_chunk);

            sion_int64 end = sion.bytes_avail_in_block();
            size_t tail_size
	      = 4 * sizeof(sion_int64) + 3 * sizeof(double);
            sion.seek(task, last_chunk, end - tail_size);

            // read tail
            sion.fread(&body_blk);
            sion.fread(&body_pos);
            sion.fread(&info_blk);
            sion.fread(&info_pos);
        }
        else
        {
            info_blk = chunkcounts[task] - 1;

            sion.seek(task, info_blk);
            info_pos = sion.bytes_avail_in_block();
        }

        SIONReader reader(sid);
        reader.seek(task, body_blk, body_pos);

        sion_int64 position_ = reader.get_position();

        sion_uint64 device_gid;
        sion_uint64 neuron_gid;
		sion_int64 step;
        double offset;
        sion_uint32 n_values;

        sion_int64 current_blk = reader.get_chunk();
        sion_int64 current_pos = reader.get_position();

        while ((current_blk < info_blk) | ((current_blk == info_blk) & (current_pos < info_pos)))
        {
            reader.read(&device_gid, 1);
            reader.read(&neuron_gid, 1);
            reader.read(&step, 1);
            reader.read(&offset, 1);
            reader.read(&n_values, 1);

            RawMemory& buffer = data_.find(device_gid)->second.data;
            buffer << (double) neuron_gid << (double) step << offset;

            for (int i = 0; i < n_values; ++i)
            {
                double value;
                reader.read(&value, 1);
                buffer << value;
            }

            current_blk = reader.get_chunk();
            current_pos = reader.get_position();
        }
    }
}

DeviceData* Reader::get_device_data(int device_gid)
{
    auto tmp = data_.find(device_gid);
    if(tmp == data_.end())
      {
	std::stringstream msg;
	msg << "unknown device gid #" << device_gid;
	throw std::out_of_range(msg.str());
      }
    return &tmp->second;
}

std::vector<int> Reader::list_devices()
{
    std::vector<int> gids;
    for (auto e = data_.cbegin(); e != data_.cend(); ++e)
    {
        gids.push_back(e->second.gid);
    }
    return gids;
}
