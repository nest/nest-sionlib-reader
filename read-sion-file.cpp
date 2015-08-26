#include <cstdio>
#include <iostream>
#include <iomanip>
#include "sion.h"

int main(int narg, char** argv)
{
	if(narg <= 1) {
		std::cerr << "usage: ./read-sion-file PATH" << std::endl;
		return 0;
	}

	char* filename = argv[1];

	sion_int64* chunk_sizes = NULL;
	sion_int32 fs_block_size;
	int n_tasks;
	int n_files;
	int* ranks = NULL;

	FILE* fh;
	
	int sid = sion_open(filename, "rb", &n_tasks, &n_files, &chunk_sizes, &fs_block_size, &ranks, &fh);

	std::cout << "number of virtual processes: " << n_tasks << std::endl;

	std::cout << std::fixed;
	std::cout << std::setprecision(3);

	for(int task=0; task < n_tasks; ++task) {
		sion_seek(sid, task, 0, 0);

		while(sion_feof(sid) < 1) {
			int device_gid;
			int neuron_gid;
			double time;
			int n_values;
			
			sion_fread(&device_gid, sizeof(int), 1, sid);
			sion_fread(&neuron_gid, sizeof(int), 1, sid);
			sion_fread(&time, sizeof(double), 1, sid);
			sion_fread(&n_values, sizeof(int), 1, sid);
			
			std::cout << device_gid << "\t" << neuron_gid << "\t" << time;

			for(int i=0; i<n_values; ++i) {
				double value;
				sion_fread(&value, sizeof(double), 1, sid);
				std::cout << "\t" << value;
			}

			std::cout << std::endl;
		}
	}
}
