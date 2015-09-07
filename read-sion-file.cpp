#include <cstdio>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <fstream>

#include "sion.h"

#include "dependencies/argparse/OptionParser.h"

class SIONFile
{
public:
    SIONFile(std::string filename) : filename_(filename)
    {
        sion_int64* chunk_sizes = NULL;
        sion_int32 fs_block_size;
        int n_tasks;
        int n_files;
        int* ranks = NULL;

        FILE* fh;

        sid_ = sion_open((char*) filename.c_str(),
            "rb",
            &n_tasks,
            &n_files,
            &chunk_sizes,
            &fs_block_size,
            &ranks,
            &fh);
    }

    void print_info()
    {
        // find tail
        int ntasks;
        int maxblocks;
        sion_int64 globalskip;
        sion_int64 start_of_varheader;
        sion_int64* localsizes;
        sion_int64* globalranks;
        sion_int64* chunkcounts;
        sion_int64* chunksizes;

        sion_get_locations(sid_,
            &ntasks,
            &maxblocks,
            &globalskip,
            &start_of_varheader,
            &localsizes,
            &globalranks,
            &chunkcounts,
            &chunksizes);

        for (int task = 0; task < ntasks; ++task)
        {
            std::cout << "\033[1m" << filename_ << " - task #" << task << "\033[0m" << std::endl;
            size_t last_chunk = chunkcounts[task] - 1;

            sion_seek(sid_, task, last_chunk, 0);
            sion_int64 end = sion_bytes_avail_in_block(sid_);

            size_t tail_size = 2 * sizeof(int) + 2 * sizeof(sion_int64) + 3 * sizeof(double);
            sion_seek(sid_, task, last_chunk, end - tail_size);

            // read tail
            int body_blk, info_blk;
            sion_int64 body_pos, info_pos;

            sion_fread(&body_blk, sizeof(int), 1, sid_);
            sion_fread(&body_pos, sizeof(sion_int64), 1, sid_);
            sion_fread(&info_blk, sizeof(int), 1, sid_);
            sion_fread(&info_pos, sizeof(sion_int64), 1, sid_);

            double t_start, t_end, duration;
            sion_fread(&t_start, sizeof(double), 1, sid_);
            sion_fread(&t_end, sizeof(double), 1, sid_);
            sion_fread(&duration, sizeof(double), 1, sid_);

            std::cout << "t_start          : " << t_start << std::endl;
            std::cout << "t_end            : " << t_end << std::endl;
            std::cout << "resolution       : " << duration << std::endl;

            // read info section
            sion_seek(sid_, task, info_blk, info_pos);

            int n_dev;
            sion_fread(&n_dev, sizeof(int), 1, sid_);
            std::cout << "number of devices: " << n_dev << std::endl;

            std::cout << std::endl;

            for (size_t i = 0; i < n_dev; ++i)
            {
                int dev_id, type;
                char name[16];
                unsigned long n_rec;
                sion_fread(&dev_id, sizeof(int), 1, sid_);
                sion_fread(&type, sizeof(int), 1, sid_);
                sion_fread(&name, sizeof(char), 16, sid_);
                sion_fread(&n_rec, sizeof(unsigned long), 1, sid_);

                if (type == 0)
                    std::cout << "spike detector ";
                else if (type == 1)
                    std::cout << "multimeter ";
                else
                    std::cout << "unknown device ";

                std::cout << "#" << dev_id << " (\"" << name << "\"):" << std::endl;
                std::cout << "  records: " << n_rec << std::endl;

                int n_val;
                sion_fread(&n_val, sizeof(int), 1, sid_);
                if (n_val > 0)
                    std::cout << "  observables:" << std::endl;
                for (size_t j = 0; j < n_val; ++j)
                {
                    char name[8];
                    sion_fread(&name, sizeof(char), 8, sid_);
                    std::cout << "    » " << name << std::endl;
                }

                std::cout << std::endl;
            }
        }
    }

    void dump()
    {
        // find tail
        int ntasks;
        int maxblocks;
        sion_int64 globalskip;
        sion_int64 start_of_varheader;
        sion_int64* localsizes;
        sion_int64* globalranks;
        sion_int64* chunkcounts;
        sion_int64* chunksizes;

        sion_get_locations(sid_,
            &ntasks,
            &maxblocks,
            &globalskip,
            &start_of_varheader,
            &localsizes,
            &globalranks,
            &chunkcounts,
            &chunksizes);

        for (int task = 0; task < ntasks; ++task)
        {
            size_t last_chunk = chunkcounts[task] - 1;

            sion_seek(sid_, task, last_chunk, 0);
            sion_int64 end = sion_bytes_avail_in_block(sid_);

            size_t tail_size = 2 * sizeof(int) + 2 * sizeof(sion_int64) + 3 * sizeof(double);
            sion_seek(sid_, task, last_chunk, end - tail_size);

            // read tail
            int body_blk, info_blk;
            sion_int64 body_pos, info_pos;

            sion_fread(&body_blk, sizeof(int), 1, sid_);
            sion_fread(&body_pos, sizeof(sion_int64), 1, sid_);
            sion_fread(&info_blk, sizeof(int), 1, sid_);
            sion_fread(&info_pos, sizeof(sion_int64), 1, sid_);

            std::map<int, std::ofstream*> files;

            // read body section
            sion_seek(sid_, task, body_blk, body_pos);

            int mc;
            sion_int64* cs;
            int current_blk = body_blk;
            sion_int64 current_pos = body_pos;

            while (
                (current_blk < info_blk) | ((current_blk == info_blk) & (current_pos < info_pos)))
            {
                int device_gid;
                int neuron_gid;
                double time;
                int n_values;

                sion_fread(&device_gid, sizeof(int), 1, sid_);
                sion_fread(&neuron_gid, sizeof(int), 1, sid_);
                sion_fread(&time, sizeof(double), 1, sid_);
                sion_fread(&n_values, sizeof(int), 1, sid_);

                if (files.find(device_gid) == files.end())
                {
                    std::stringstream filename;
                    filename << "device-" << device_gid << "-" << task << ".dat";

                    std::ofstream* file = new std::ofstream();
                    file->open(filename.str().c_str());
                    (*file) << std::fixed << std::setprecision(5);
                    files.insert(std::pair<int, std::ofstream*>(device_gid, file));
                }

                std::ofstream* file = files.find(device_gid)->second;
                (*file) << device_gid << "\t" << neuron_gid << "\t" << time;

                for (int i = 0; i < n_values; ++i)
                {
                    double value;
                    sion_fread(&value, sizeof(double), 1, sid_);
                    (*file) << "\t" << value;
                }
                (*file) << "\n";

                sion_get_current_location(sid_, &current_blk, &current_pos, &mc, &cs);
            }

            for (std::map<int, std::ofstream*>::iterator it = files.begin(); it != files.end();
                 ++it)
            {
                it->second->close();
                delete it->second;
            }
        }
    }

private:
    std::string filename_;
    int sid_;
};

int main(int argc, char** argv)
{
    using optparse::OptionParser;
    OptionParser parser = OptionParser().description("read and analyze SIONlib output from NEST::");
    parser.usage("read-sion-file [options] FILE");

    parser.add_option("-i", "--info")
        .action("store_true")
        .dest("info")
        .set_default("0")
        .help("print file info and meta data to screen");

    parser.add_option("-d", "--dump")
        .action("store_true")
        .dest("dump")
        .set_default("0")
        .help("dump recorded data to individual files per recording device and task");

    optparse::Values options = parser.parse_args(argc, argv);
    std::vector<std::string> args = parser.args();

    for (std::vector<std::string>::iterator f = args.begin(); f != args.end(); ++f)
    {
        SIONFile file(*f);

        if (options.get("info"))
            file.print_info();
        if (options.get("dump"))
            file.dump();
    }
}
