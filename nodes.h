#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

struct file_d {
    char filename[64];
    uint32_t offset;
    uint32_t length;
    
};

struct helper_d {
    FILE* file_data_ptr;
    FILE* directory_ptr;
    FILE* hash_data_ptr;
    struct repack_file_d * file_array;
    void * temp;
    void * temp2;
	unsigned int n_of_files;
    unsigned int MAX_N_FILES;
    unsigned int MAX_DATA_SPACE;
    pthread_mutex_t * lock;
};

struct repack_file_d {
	char filename[64];
	uint32_t offset;
    uint32_t length;
	uint32_t directory_offset;
	
};

