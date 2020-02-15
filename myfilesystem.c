#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include "myfilesystem.h"
#include "nodes.h"

void update_hash_file(int length, int offset, struct helper_d * helper) {
    uint32_t block_offset = offset / 256;
    while (length > 0) {
        compute_hash_block_helper(block_offset, helper);
        length = length - 256 + (offset % 256);
        offset = 0;
        if (block_offset % 2 == 0) {
            block_offset++;
            length -= 256;
        }
        block_offset++;
    }
    return;
}

void update_file_array(struct helper_d * help) {
    struct file_d temp_file;
    struct repack_file_d file;
	int finish = 1;
	int count = 0;
    int file_n = 0;
	// read directory and store file details in array
    fseek(help->directory_ptr, 0, SEEK_SET);
	while (finish) {
		if (ftell(help->directory_ptr) == help->MAX_N_FILES * 72) {
			finish = 0;
			break;
		}
		fread(&temp_file, sizeof(struct file_d), 1, help->directory_ptr);
		if (*(temp_file.filename) == '\0') {
			count++;
			continue;
		}
        for (int i = 0; i < 64; i++) {
            file.filename[i] = (temp_file.filename)[i];
        }
        file.offset = temp_file.offset;
        file.length = temp_file.length;
		file.directory_offset = count * 72;
		*((help->file_array) + (file_n)) = file;
		file_n++;
		count++;
    }
	// sort file array by offset
	unsigned int key;
    int j;
	for (int i = 1; i < file_n; i++) {
		file = *((help->file_array) + i);
		key = file.offset;
		j = i - 1;
		while (j >= 0 && (*((help->file_array) + j)).offset > key) {
            *((help->file_array) + (j + 1)) = *((help->file_array) + j);
            j = j - 1;
		}
		*((help->file_array) + (j + 1)) = file;
	}
    help->n_of_files = file_n;
}

void * init_fs(char * f1, char * f2, char * f3, int n_processors) {
    // open given files for reading and writing
    FILE* file_data = fopen(f1, "rb+");
    FILE* directory = fopen(f2, "rb+");
    FILE* hash_data = fopen(f3, "rb+");
    // construct helper and fill in contents
    struct helper_d* helper = (struct helper_d *) malloc(sizeof(struct helper_d));
    helper->file_data_ptr = file_data;
    helper->directory_ptr = directory;
    helper->hash_data_ptr = hash_data;
	helper->temp = NULL;
    helper->temp2 = NULL;
    fseek(helper->file_data_ptr, 0L, SEEK_END);
    helper->MAX_DATA_SPACE = ftell(helper->file_data_ptr);
    // set up mutex locks
    helper->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(helper->lock, NULL);
	// fill in file array
    fseek(helper->directory_ptr, 0L, SEEK_END);
    helper->MAX_N_FILES = ftell(helper->directory_ptr) / 72;
    helper->file_array = (struct repack_file_d *) malloc(sizeof(struct repack_file_d) * (helper->MAX_N_FILES));
    helper->n_of_files = 0;
    update_file_array(helper);
    // set helper to of void * type
    void * help = (void *) helper;
    //compute_hash_tree(help);
    return help;
}

void close_fs(void * helper) {
    // return helper to of helper_d * type and free all mallocs
    struct helper_d * help = (struct helper_d *) helper;
    free(help->file_array);
    // close the opened files
    fclose(help->file_data_ptr);
    fclose(help->directory_ptr);
    fclose(help->hash_data_ptr);
    // destroy mutex locks
    pthread_mutex_destroy(help->lock);
    free(help->lock);
    // free helper
    free(help);
    help = NULL;
    return;
}

int create_file(char * filename, size_t length, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    struct repack_file_d file;
    int directory_offset = 0;
    int data_offset = 0;
    int stored_data_size = 0;
    int repack_needed = 1;
    pthread_mutex_lock(help->lock);
    if ((help->n_of_files) == help->MAX_N_FILES) {
        // directory table is full
        pthread_mutex_unlock(help->lock);
        return 2;
    }
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (strcmp(file.filename, filename) == 0) {
            // file already exists in system
            pthread_mutex_unlock(help->lock);
            return 1;
        }
        if (file.directory_offset == directory_offset * 72) {
            directory_offset++;
        }
        if (data_offset + (uint32_t)length > file.offset) {
            data_offset = file.offset + file.length;
        } else {
            repack_needed = 0;
        }
        stored_data_size += file.length;
    }
    if (data_offset + length <= help->MAX_DATA_SPACE) {
        repack_needed = 0;
    }
    // check for sufficient space in disk
    if (stored_data_size + length > help->MAX_DATA_SPACE) {
        pthread_mutex_unlock(help->lock);
        return 2;
    }
    if (repack_needed) {
        repack_helper(helper);
        data_offset = stored_data_size;
    }
    // create new file in directory table
    struct file_d new_file;
    int i;
    for (i = 0; i < strlen(filename); i++) {
        new_file.filename[i] = filename[i];
    }
    new_file.filename[i] = '\0';
    new_file.length = length;
    new_file.offset = data_offset;
    fseek(help->directory_ptr, directory_offset * 72, SEEK_SET);
    fwrite(&new_file, sizeof(struct file_d), 1, help->directory_ptr);
    // update file data file
    int zero[new_file.length];
    for (int i = 0; i < new_file.length; i++) {
        zero[i] = 0;
    }
    fseek(help->file_data_ptr, new_file.offset, SEEK_SET);
    fwrite(zero, new_file.length, 1, help->file_data_ptr);
    // update hash file
    compute_hash_tree_helper(helper);
    update_file_array(help);
    fflush(help->directory_ptr);
    pthread_mutex_unlock(help->lock);
    return 0;
}

int resize_file(char * filename, size_t length, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    pthread_mutex_lock(help->lock);
    int ret = resize_file_helper(filename, length, helper);
    pthread_mutex_unlock(help->lock);
    return ret;
}

int resize_file_helper(char * filename, size_t length, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    struct repack_file_d file;
    struct repack_file_d resize_file;
    struct repack_file_d next_file;
    struct file_d new_file;
    int found = 0;
    int last_file = 0;
    int stored_data_size = 0;
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (strcmp(file.filename, filename) == 0) {
            // found file
            found = 1;
            resize_file = *((help->file_array) + i);
            for (int i = 0; i < 64; i++) {
                    new_file.filename[i] = (resize_file.filename)[i];
                }
            new_file.length = length;
            new_file.offset = resize_file.offset;
            if (length < resize_file.length) {
                // decreasing file size, no need to check for sufficient space
                fseek(help->directory_ptr, resize_file.directory_offset, SEEK_SET);
                fwrite(&new_file, sizeof(struct file_d), 1, help->directory_ptr);
                update_file_array(help);
                fflush(help->directory_ptr);
                return 0;
            }
            if (i != help->n_of_files - 1) {
                // found file is not the last file in array
                next_file = *((help->file_array) + (i + 1));
            } else {
                last_file = 1;
            }
        } else {
            stored_data_size += file.length;
        }
    } 
    if (found == 0) {
        // file not in system
        return 1;
    }
    // check if sufficient space in virtual disk
    if (stored_data_size + length > help->MAX_DATA_SPACE) {
        return 2;
    }
    // check if sufficient space in between next file/end of file_data file and repack if needed
    int check_value;
    if (last_file) {
        check_value = help->MAX_DATA_SPACE;
    } else {
        check_value = next_file.offset;
    }
    char buf[resize_file.length];
    fseek(help->file_data_ptr, resize_file.offset, SEEK_SET);
    fread(&buf, resize_file.length, 1, help->file_data_ptr);
    if (resize_file.offset + length >= check_value) {
        help->temp = resize_file.filename;
        repack_helper(helper);
        new_file.offset = stored_data_size;
        help->temp2 = &stored_data_size;
        help->temp = NULL;
    }
    // update data file
    if (resize_file.offset >= stored_data_size) {
        char zero[resize_file.length];
        memset(zero, 0, resize_file.length);
        fseek(help->file_data_ptr, resize_file.offset, SEEK_SET);
        fwrite(&zero, resize_file.length, 1, help->file_data_ptr);
    }
    fseek(help->file_data_ptr, new_file.offset, SEEK_SET);
    fwrite(&buf, resize_file.length, 1, help->file_data_ptr);
    
    // update directory file
    fseek(help->directory_ptr, resize_file.directory_offset, SEEK_SET);
    fwrite(&new_file, sizeof(struct file_d), 1, help->directory_ptr);
    // update hash data file
    compute_hash_tree_helper(helper);
    update_file_array(help);
    fflush(help->directory_ptr);
    fflush(help->file_data_ptr);
    return 0;
}

void repack(void * helper) {
	struct helper_d * help = (struct helper_d *) helper;
    struct repack_file_d file;
    int ptr = 0;
    int ignore_file = 0;
    char * filename;
    pthread_mutex_lock(help->lock);
    if (help->temp != NULL) {
        filename = (char *) help->temp;
        ignore_file = 1;
    }
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (ignore_file) {
            if (strcmp(filename, file.filename) == 0) {
                continue;
            }
        }
        // check if file is already at the left most position
        if (file.offset == ptr) {
            ptr = file.offset + file.length;
            continue;
        }
        // update file_data file (read in original contents)
        char buf[file.length];
        fseek(help->file_data_ptr, file.offset, SEEK_SET);
        fread(buf, file.length, 1, help->file_data_ptr);
        // update file_data file (write back in right place)
        if (file.offset != ptr) {
            fseek(help->file_data_ptr, ptr, SEEK_SET);
            fwrite(buf, file.length, 1, help->file_data_ptr);
            fflush(help->file_data_ptr);
            memset(buf, 0, file.length);
            fseek(help->file_data_ptr, file.offset, SEEK_SET);
            fwrite(&buf, file.length, 1, help->file_data_ptr);
            fflush(help->file_data_ptr);
        }
        compute_hash_tree_helper(helper);
        // update directory file
        file.offset = ptr;
        fseek(help->directory_ptr, file.directory_offset + 64, SEEK_SET);
        fwrite(&ptr, sizeof(ptr), 1, help->directory_ptr);
        fflush(help->directory_ptr);
        ptr = file.offset + file.length;
    }
    update_file_array(help);
    fflush(help->directory_ptr);
    fflush(help->file_data_ptr);
    pthread_mutex_unlock(help->lock);
    return;
}

void repack_helper(void * helper) {
	struct helper_d * help = (struct helper_d *) helper;
    struct repack_file_d file;
    unsigned ptr = 0;
    unsigned ignore_file = 0;
    char * filename;
    if (help->temp != NULL) {
        filename = (char *) help->temp;
        ignore_file = 1;
    }
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (ignore_file) {
            if (strcmp(filename, file.filename) == 0) {
                continue;
            }
        }
        // check if file is already at the left most position
        if (file.offset == ptr) {
            ptr = file.offset + file.length;
            continue;
        }
        // update file_data file (read in original contents)
        char buf[file.length];
        fseek(help->file_data_ptr, file.offset, SEEK_SET);
        fread(buf, file.length, 1, help->file_data_ptr);
        // update file_data file (write back in right place)
        if (file.offset != ptr) {
            fseek(help->file_data_ptr, ptr, SEEK_SET);
            fwrite(buf, file.length, 1, help->file_data_ptr);
        }
        // update directory file
        file.offset = ptr;
        fseek(help->directory_ptr, file.directory_offset + 64, SEEK_SET);
        fwrite(&ptr, 4, 1, help->directory_ptr);
        
        fflush(help->directory_ptr);
        ptr += file.length;
    }
    unsigned length = help->MAX_DATA_SPACE - ptr;
    char zero[length];
    memset(zero, 0, length);
    fseek(help->file_data_ptr, ptr, SEEK_SET);
    fwrite(zero, length, 1, help->file_data_ptr);
    update_file_array(help);
    fflush(help->directory_ptr);
    fflush(help->file_data_ptr);
    return;
}

int delete_file(char * filename, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    struct repack_file_d file;
    pthread_mutex_lock(help->lock);
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (strcmp(file.filename, filename) == 0) {
            // found file, change filename to '\0'
            fseek(help->directory_ptr, file.directory_offset, SEEK_SET);
            char null = '\0';
            fwrite(&null, sizeof(char), 1, help->directory_ptr);
            update_file_array(help);
            fflush(help->directory_ptr);
            pthread_mutex_unlock(help->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(help->lock);
    return 1;
}

int rename_file(char * oldname, char * newname, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    struct repack_file_d file;
    struct file_d new_file;
    int found = 0;
    int directory_offset = 0;
    pthread_mutex_lock(help->lock);
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (strcmp(file.filename, newname) == 0) {
            // file of newname already exists
            pthread_mutex_unlock(help->lock);
            return 1;
        } else if (strcmp(file.filename, oldname) == 0) {
            // found file
            found = 1;
            int i;
            for (i = 0; i < strlen(newname); i++) {
                new_file.filename[i] = newname[i];
            }
            new_file.filename[i] = '\0';
            new_file.length = file.length;
            new_file.offset = file.offset;
            directory_offset = file.directory_offset;
        }
    }
    if (found) {
        fseek(help->directory_ptr, directory_offset, SEEK_SET);
        fwrite(&new_file, sizeof(struct file_d), 1, help->directory_ptr);
        update_file_array(help);
        fflush(help->directory_ptr);
        pthread_mutex_unlock(help->lock);
        return 0;
    }
    pthread_mutex_unlock(help->lock);
    return 1;
}

int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    // search for file in file array
    struct repack_file_d file;
    pthread_mutex_lock(help->lock);
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (strcmp(file.filename, filename) == 0) {
            // found file, read file
            if (offset + count > file.length) {
                // given offset is longer than the file length
                pthread_mutex_unlock(help->lock);
                return 2;
            }
            fseek(help->file_data_ptr, offset + file.offset, SEEK_SET);
            fread(buf, count, 1, help->file_data_ptr);
            // verify correctness of hash values
            int iterations = 0;
            uint8_t * temp = (uint8_t *) malloc(256);
            uint8_t * buf = (uint8_t *) malloc(sizeof(uint8_t) * 16);
            uint8_t * output = (uint8_t *) malloc(sizeof(uint8_t) * 16);
            uint8_t * big_buf = (uint8_t *) malloc(sizeof(uint8_t) * 32);
            int c = (int) count;
            while (c > 0) {
                uint32_t block_offset = (offset / 256) + iterations;
                int level = (int) (log((help->MAX_DATA_SPACE)/256) / log(2));
                uint32_t counter = (uint32_t) pow(2, level) - 1 + block_offset;
                if (iterations == 0) {
                    c = c - 256 + (offset % 256);
                } else {
                    c -= 256;
                }
                fseek(help->file_data_ptr, block_offset * 256, SEEK_SET);
                fread(temp, 256, 1, help->file_data_ptr);
                fletcher(temp, 256, output);
                fseek(help->hash_data_ptr, 16 * (block_offset + (int)pow(2, level) - 1), SEEK_SET);
                fread(buf, 16, 1, help->hash_data_ptr);
                for (int j = 0; j < 16; j++) {
                    if (buf[j] != output[j]) {
                        free(temp);
                        free(buf);
                        free(big_buf);
                        free(output);
                        pthread_mutex_unlock(help->lock);
                        return 3;
                    }
                }
                while (level > 0) {
                    if (counter % 2 == 0) {
                        fseek(help->hash_data_ptr, 16 * (counter - 1), SEEK_SET);
                        counter = counter / 2 - 1;
                    } else {
                        fseek(help->hash_data_ptr, 16 * counter, SEEK_SET);
                        counter = (counter - 1) / 2;
                        c -= 256;
                        iterations++;
                    }
                    fread(big_buf, 32, 1, help->hash_data_ptr);
                    fseek(help->hash_data_ptr, 16 * counter, SEEK_SET);
                    fread(buf, 16, 1, help->hash_data_ptr);
                    fletcher(big_buf, 32, output);
                    for (int j = 0; j < 16; j++) {
                        if (buf[j] != output[j]) {
                            free(temp);
                            free(buf);
                            free(big_buf);
                            free(output);
                            pthread_mutex_unlock(help->lock);
                            return 3;
                        }
                    }
                    level -= 1;
                }
                iterations++;
            }
            free(temp);
            free(buf);
            free(big_buf);
            free(output);
            pthread_mutex_unlock(help->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(help->lock);
    return 1;
}

int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    pthread_mutex_lock(help->lock);
    // search for file in file array
    struct repack_file_d file;
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (strcmp(file.filename, filename) == 0) {
            // found file
            if ((uint32_t) offset > file.length) {
                // offset is set to a value larger than the file length
                pthread_mutex_unlock(help->lock);
                return 2;
            }
            if ((uint32_t) offset + (uint32_t) count > file.length) {
                // data to be written exceeds file size
                int result = resize_file_helper(filename, (uint32_t) offset + (uint32_t) count, helper);
                if (result == 0) {
                    // file is successfully increased to needed size
                    if (help->temp2 != NULL) {
                        file.offset = *(uint32_t *)(help->temp2);
                        help->temp2 = NULL;
                    }
                    fseek(help->file_data_ptr, file.offset + offset, SEEK_SET);
                    fwrite(buf, count, 1, help->file_data_ptr);
                    update_hash_file(count, file.offset + offset, help);
                    fflush(help->file_data_ptr);
                    pthread_mutex_unlock(help->lock);
                    return 0;
                } else {
                    // not enough space in virtual disk
                    pthread_mutex_unlock(help->lock);
                    return 3;
                }
            } else {
                // data to be written doesn't exceed file size, can write directly
                fseek(help->file_data_ptr, file.offset + offset, SEEK_SET);
                fwrite(buf, count, 1, help->file_data_ptr);
                update_hash_file(count, file.offset + offset, help);
                fflush(help->file_data_ptr);
                pthread_mutex_unlock(help->lock);
                return 0;
            }
        }
    }
    pthread_mutex_unlock(help->lock);
    return 1;
}

ssize_t file_size(char * filename, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    struct repack_file_d file;
    pthread_mutex_lock(help->lock);
    // read directory file to get details of all files
    for (int i = 0; i < help->n_of_files; i++) {
        file = *((help->file_array) + i);
        if (strcmp(file.filename, filename) == 0) {
            pthread_mutex_unlock(help->lock);
            return file.length;
        }
    }
    pthread_mutex_unlock(help->lock);
    return -1;
}

void fletcher(uint8_t * buf, size_t length, uint8_t * output) {
    size_t size = -1;
    if (length % 4 == 0) {
        size = length/4;
    } else {
        size = length/4 + 1;
    }
    uint32_t new_buf[size];
    memset(new_buf, 0, size);
    memcpy(new_buf, buf, length);
    uint64_t a = 0;
    uint64_t b = 0;
    uint64_t c = 0;
    uint64_t d = 0;
    for (int i = 0; i < size; i++) {
        a = (a + new_buf[i]) % (uint64_t)(pow(2, 32) - 1);
        b = (a + b) % (uint64_t)(pow(2, 32) - 1);
        c = (b + c) % (uint64_t)(pow(2, 32) - 1);
        d = (c + d) % (uint64_t)(pow(2, 32) - 1);
    }
    uint32_t aa = (uint32_t) a;
    uint32_t bb = (uint32_t) b;
    uint32_t cc = (uint32_t) c;
    uint32_t dd = (uint32_t) d;
    memcpy(output, &aa, 4);
    memcpy(output + 4, &bb, 4);
    memcpy(output + 8, &cc, 4);
    memcpy(output + 12, &dd, 4);
    return;
}

void compute_hash_tree(void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    pthread_mutex_lock(help->lock);
    compute_hash_tree_helper(helper);
    pthread_mutex_unlock(help->lock);
    return;
}

void compute_hash_tree_helper(void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    uint8_t * buf = (uint8_t *) malloc(256);
    uint8_t * output = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    int level = (int) (log((help->MAX_DATA_SPACE)/256) / log(2));
    int count = 0;
    fseek(help->file_data_ptr, 0, SEEK_SET);
    for (int i = 0; i < (help->MAX_DATA_SPACE)/256; i++) {
        fseek(help->file_data_ptr, i * 256, SEEK_SET);
        fread(buf, 256, 1, help->file_data_ptr);
        fletcher(buf, 256, output);
        fseek(help->hash_data_ptr, 16 * (count + (int)pow(2, level) - 1), SEEK_SET);
        fwrite(output, 16, 1, help->hash_data_ptr);
        count++;
    }
    uint8_t* small_buf = (uint8_t*) malloc(32);
    while (level >= 0) {
        level -= 1;
        count = count/2;
        for (int i = 0; i < count; i++) {
            fseek(help->hash_data_ptr, 16 * (2 * i + (int)pow(2, level+1) - 1), SEEK_SET);
            fread(small_buf, 32, 1, help->hash_data_ptr);
            fletcher(small_buf, 32, output);
            fseek(help->hash_data_ptr, 16 * (i + (int)pow(2, level) - 1), SEEK_SET);
            fwrite(output, 16, 1, help->hash_data_ptr);
        }
    }
    free(buf);
    free(small_buf);
    free(output);
    fflush(help->hash_data_ptr);
    return;
}

void compute_hash_block(size_t block_offset, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    pthread_mutex_lock(help->lock);
    compute_hash_block_helper(block_offset, helper);
    pthread_mutex_unlock(help->lock);
    return;
}

void compute_hash_block_helper(size_t block_offset, void * helper) {
    struct helper_d * help = (struct helper_d *) helper;
    uint8_t * buf = (uint8_t *) malloc(256);
    uint8_t * output = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    int level = (int) (log((help->MAX_DATA_SPACE)/256) / log(2));
    int counter = block_offset + (int)pow(2, level) - 1;
    fseek(help->file_data_ptr, block_offset * 256, SEEK_SET);
    fread(buf, 256, 1, help->file_data_ptr);
    fletcher(buf, 256, output);
    fseek(help->hash_data_ptr, 16 * counter, SEEK_SET);
    fwrite(output, 16, 1, help->hash_data_ptr);
    uint8_t values[32];
    while (level > 0) {
        level -= 1;
        if (counter % 2 == 0) {
            fseek(help->hash_data_ptr, 16 * (counter - 1), SEEK_SET);
            counter = counter / 2 - 1;
        } else {
            fseek(help->hash_data_ptr, 16 * counter, SEEK_SET);
            counter = (counter - 1) / 2;
        }
        fread(values, 32, 1, help->hash_data_ptr);
        fletcher(values, 32, output);
        fseek(help->hash_data_ptr, 16 * counter, SEEK_SET);
        fwrite(output, 16, 1, help->hash_data_ptr);
    }
    fflush(help->hash_data_ptr);
    fseek(help->hash_data_ptr, 0, SEEK_SET);
    fread(output, 16, 1, help->hash_data_ptr);
    free(buf);
    free(output);
    return;
}
