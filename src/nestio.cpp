#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cstddef>

#include "nestio.h"

enum {LITTLE_ENDIAN = 0, BIG_ENDIAN = 1} sion_endian;

static int16_t z1 = 1;
sion_endian my_endian = ((static_cast<char*>(&z1))[0] == 0)
  ? BIG_ENDIAN : LITTLE_ENDIAN;

size_t normal_fread(const void* data, size_t size, size_t nitems, int sid);
size_t swapped_fread(const void* data, size_t size, size_t nitems, int sid);

size_t (*freader)(const void* data, size_t size, size_t nitems, int sid);

size_t normal_fread(const void* data, size_t size, size_t nitems, int sid) {
  return sion_fread(data, size, nitems, side);
}

template<size_t size>
static void swap(const char* subbuf) {
  for (size_t j = 0, jp = size-1; j < size/2; j++, jp--) {
    char jt = subbuf[j];
    subbuf[j] = subbuf[jp];
    subbuf[jp] = jt;
  }
}

template void swap(const char* buf) {}

template<size_t size>
static void swap_loop(const char* buf, size_t nitems) {
  for (size_t i = 0; i < size*nitems; i += size) {
    swap<size>(buf+i)
  }
}

static void swap_switch(const char* buf, size_t size, size_t nitems) {
  switch (size) {
    case 1:                              return;
    case 2:  swap_loop< 2>(buf, nitems); return;
    case 4:  swap_loop< 4>(buf, nitems); return;
    case 8:  swap_loop< 8>(buf, nitems); return;
    case 16: swap_loop<16>(buf, nitems); return;
  }

  std::cerr << __FILE__ << ", " << __LINE__ << ": swap_switch illegal size " << size << std::endl;
  std::abort();
}

static size_t swapped_fread(const void* data, size_t size, size_t nitems, int sid, bool swap) {
  size_t r = sion_fread(data, size, nitems, sid);
  if (swap) swap_switch(static_cast<char*>(data), size, nitems);
  return r;
}


Reader::Reader(std::string filename)
{
    sion_int64* chunk_sizes = NULL;
    sion_int32 fs_block_size;
    int n_tasks;
    int n_files;
    int* ranks = NULL;

    FILE* fh;

    int sid = sion_open((char*) filename.c_str(),
        "rb",
        &n_tasks,
        &n_files,
        &chunk_sizes,
        &fs_block_size,
        &ranks,
        &fh);

    bool swap = sion_get_file_endianness(sid) == my_endian;

    // find tail
    int ntasks;
    int maxblocks;
    sion_int64 globalskip;
    sion_int64 start_of_varheader;
    sion_int64* localsizes;
    sion_int64* globalranks;
    sion_int64* chunkcounts;
    sion_int64* chunksizes;

    sion_get_locations(sid,
        &ntasks,
        &maxblocks,
        &globalskip,
        &start_of_varheader,
        &localsizes,
        &globalranks,
        &chunkcounts,
        &chunksizes);

    int task = 0;

    size_t last_chunk = chunkcounts[task] - 1;

    sion_seek(sid, task, last_chunk, 0);
    sion_int64 end = sion_bytes_avail_in_block(sid);

    size_t tail_size = 4 * sizeof(sion_int64) + 3 * sizeof(double);

    // check if tail section was splitted over two chunks and fix seeking
    if (tail_size > end)
    {
        last_chunk--;
        tail_size -= end;
        sion_seek(sid, task, last_chunk, 0);
        end = sion_bytes_avail_in_block(sid);
    }
    sion_seek(sid, task, last_chunk, end - tail_size);

    // read tail
    sion_int64 body_blk, info_blk, body_pos, info_pos;

    swapped_fread(&body_blk, sizeof(sion_int64), 1, sid, swap);
    swapped_fread(&body_pos, sizeof(sion_int64), 1, sid, swap);
    swapped_fread(&info_blk, sizeof(sion_int64), 1, sid, swap);
    swapped_fread(&info_pos, sizeof(sion_int64), 1, sid, swap);

    double t_start, t_end, duration;
    swapped_fread(&t_start, sizeof(double), 1, sid, swap);
    swapped_fread(&t_end, sizeof(double), 1, sid, swap);
    swapped_fread(&duration, sizeof(double), 1, sid, swap);

    // read info section
    sion_seek(sid, task, info_blk, info_pos);

    int n_dev;
    swapped_fread(&n_dev, sizeof(sion_uint64), 1, sid, swap);

    for (size_t i = 0; i < n_dev; ++i)
    {
        sion_uint64 dev_id;
		sion_uint32 type;
        char name[16];
        char label[16];
        swapped_fread(&dev_id, sizeof(sion_uint64), 1, sid, swap);
        swapped_fread(&type, sizeof(sion_uint32), 1, sid, swap);
        swapped_fread(&name, sizeof(char), 16, sid, swap);
        swapped_fread(&label, sizeof(char), 16, sid, swap);

        int n_rec;
        swapped_fread(&n_rec, sizeof(sion_uint64), 1, sid, swap);

        int n_val;
        swapped_fread(&n_val, sizeof(sion_uint32), 1, sid, swap);

		std::vector<std::string> observables;
        for (size_t j = 0; j < n_val; ++j)
        {
            char name[8];
            swapped_fread(&name, sizeof(char), 8, sid, swap);
            observables.push_back(name);
        }

        size_t entry_size = sizeof(double) + sizeof(double) + n_val * sizeof(double);
        std::pair<size_t, size_t> shape = std::make_pair(n_rec, n_val + 3);

        DeviceData& entry = data_.insert(std::make_pair(dev_id, DeviceData(shape))).first->second;
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

            sion_seek(sid, task, last_chunk, 0);

            sion_int64 end = sion_bytes_avail_in_block(sid, swap);

            size_t tail_size = 4 * sizeof(sion_int64) + 3 * sizeof(double);
            sion_seek(sid, task, last_chunk, end - tail_size);

            // read tail
            swapped_fread(&body_blk, sizeof(sion_int64), 1, sid, swap);
            swapped_fread(&body_pos, sizeof(sion_int64), 1, sid, swap);
            swapped_fread(&info_blk, sizeof(sion_int64), 1, sid, swap);
            swapped_fread(&info_pos, sizeof(sion_int64), 1, sid, swap);
        }
        else
        {
            info_blk = chunkcounts[task] - 1;

            sion_seek(sid, task, info_blk, 0);
            info_pos = sion_bytes_avail_in_block(sid, swap);
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
