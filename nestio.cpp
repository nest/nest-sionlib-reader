#include "nestio.h"

SIONFile::SIONFile(std::string filename)
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

    size_t tail_size = 2 * sizeof(int) + 2 * sizeof(sion_int64) + 3 * sizeof(double);

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
    int body_blk, info_blk;
    sion_int64 body_pos, info_pos;

    sion_fread(&body_blk, sizeof(int), 1, sid);
    sion_fread(&body_pos, sizeof(sion_int64), 1, sid);
    sion_fread(&info_blk, sizeof(int), 1, sid);
    sion_fread(&info_pos, sizeof(sion_int64), 1, sid);

    double t_start, t_end, duration;
    sion_fread(&t_start, sizeof(double), 1, sid);
    sion_fread(&t_end, sizeof(double), 1, sid);
    sion_fread(&duration, sizeof(double), 1, sid);

    // read info section
    sion_seek(sid, task, info_blk, info_pos);

    int n_dev;
    sion_fread(&n_dev, sizeof(int), 1, sid);

    for (size_t i = 0; i < n_dev; ++i)
    {
        int dev_id, type;
        char name[16];
        char label[16];
        sion_fread(&dev_id, sizeof(int), 1, sid);
        sion_fread(&type, sizeof(int), 1, sid);
        sion_fread(&name, sizeof(char), 16, sid);
        sion_fread(&label, sizeof(char), 16, sid);

        int n_rec;
        sion_fread(&n_rec, sizeof(int), 1, sid);

        int n_val;
        sion_fread(&n_val, sizeof(int), 1, sid);
        
		std::vector<std::string> observables;
        for (size_t j = 0; j < n_val; ++j)
        {
            char name[8];
            sion_fread(&name, sizeof(char), 8, sid);
            observables.push_back(name);
        }

        size_t entry_size = sizeof(double) + sizeof(double) + n_val * sizeof(double);

        std::pair<size_t, size_t> shape = std::make_pair(n_rec, n_val + 2);

        DeviceData& entry = data_.insert(std::make_pair(dev_id, DeviceData(shape))).first->second;
        entry.gid = dev_id;
        entry.name = name;
        entry.label = label;
        entry.observables = observables;
    }

    for (int task = 0; task < ntasks; ++task)
    {
        int body_blk, info_blk;
        sion_int64 body_pos, info_pos;

        if (task == 0)
        {
            size_t last_chunk = chunkcounts[task] - 1;

            sion_seek(sid, task, last_chunk, 0);

            sion_int64 end = sion_bytes_avail_in_block(sid);

            size_t tail_size = 2 * sizeof(int) + 2 * sizeof(sion_int64) + 3 * sizeof(double);
            sion_seek(sid, task, last_chunk, end - tail_size);

            // read tail
            sion_fread(&body_blk, sizeof(int), 1, sid);
            sion_fread(&body_pos, sizeof(sion_int64), 1, sid);
            sion_fread(&info_blk, sizeof(int), 1, sid);
            sion_fread(&info_pos, sizeof(sion_int64), 1, sid);
        }
        else
        {
            info_blk = chunkcounts[task] - 1;

            sion_seek(sid, task, info_blk, 0);
            info_pos = sion_bytes_avail_in_block(sid);
        }

        SIONReader reader(sid);
        reader.seek(task, body_blk, body_pos);

        sion_int64 position_ = reader.get_position();

        int device_gid;
        int neuron_gid;
        double time;
        int n_values;

        sion_int64 current_blk = reader.get_chunk();
        sion_int64 current_pos = reader.get_position();

        while ((current_blk < info_blk) | ((current_blk == info_blk) & (current_pos < info_pos)))
        {
            reader.read(&device_gid, 1);
            reader.read(&neuron_gid, 1);
            reader.read(&time, 1);
            reader.read(&n_values, 1);

            RawMemory& buffer = data_.find(device_gid)->second.data;
            buffer << (double) neuron_gid << time;

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

DeviceData& SIONFile::get_device_data(int device_gid)
{
    return data_.find(device_gid)->second;
}

std::vector<int> SIONFile::list_devices()
{
    std::vector<int> gids;
    for (auto e = data_.cbegin(); e != data_.cend(); ++e)
    {
        gids.push_back(e->second.gid);
    }
    return gids;
}
