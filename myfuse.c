/* Do not change! */
#define FUSE_USE_VERSION 29
#define _FILE_OFFSET_BITS 64
/******************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fuse.h>
#include <errno.h>
#include "nodes.h"
#include "myfilesystem.h"


char * file_data_file_name = NULL;
char * directory_table_file_name = NULL;
char * hash_data_file_name = NULL;

int myfuse_getattr(const char * filename, struct stat * result) {
    // MODIFY THIS FUNCTION
    memset(result, 0, sizeof(struct stat));
    if (strcmp(filename, "/") == 0) {
        result->st_mode = S_IFDIR;
	return 0;
    } else {
        result->st_mode = S_IFREG;
    }
    void * helper = fuse_get_context()->private_data;
    int name_length = strlen(filename);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = filename[i+1];
    }
    ssize_t ret = file_size(name, helper);
    if (ret == -1) {
	    errno = ENOENT;
        return -errno;
    }
	result->st_size = ret;
    return 0;
}

int myfuse_readdir(const char * filename, void * buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {
    // MODIFY THIS FUNCTION
    if (strcmp(filename, "/") == 0) {
	struct helper_d * helper = fuse_get_context()->private_data;
	struct repack_file_d * file_array = helper->file_array;
	for (int i = 0; i < (helper->n_of_files); i++) {
		filler(buf, file_array[i].filename, NULL, 0);
	}
	return 0;
    } else {
        return -errno;
    }
}

int myfuse_unlink(const char * filename) {
    void * helper = fuse_get_context()->private_data;
    int name_length = strlen(filename);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = filename[i+1];
    }
    int ret = delete_file(name, helper);
    if (ret == 0) {
       return 0;
    }
    return -errno;
}
    // FILL OUT

int myfuse_rename(const char * oldname, const char * newname) {
    void * helper = fuse_get_context()->private_data;
    int name_length = strlen(oldname);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = oldname[i+1];
    }
    name_length = strlen(newname);
    char new[name_length];
    for (int i = 0; i < name_length; i++) {
        new[i] = newname[i+1];
    }
    int ret = rename_file(name, new, helper);
	if (ret == 0) {
		return 0;
	}
    return -errno;
}
    // FILL OUT

int myfuse_truncate(const char * filename, off_t length) {
    void * helper = fuse_get_context()->private_data;
    int name_length = strlen(filename);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = filename[i+1];
    }
    int ret = resize_file(name, length, helper);
    if (ret == 1) {
        return -errno;
    } else if (ret == 2) {
        return -errno;
    } else if (ret == 0) {
        return 0;
    }
    return -errno;
}
    // FILL OUT

int myfuse_open(const char * filename, struct fuse_file_info * fi) {
    void * helper = fuse_get_context()->private_data;
    int name_length = strlen(filename);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = filename[i+1];
    }
    int ret = file_size(name, helper);
    if (ret == 1) {
        return -errno;
    } else if (ret == 0)  {
        fi->fh = 1;
        return 0;
    }
    return -errno;
}
    // FILL OUT

int myfuse_read(const char * filename, char * buf, size_t length, off_t offset, struct fuse_file_info * fi) {
    if (fi->fh == 0) {
        errno = EBADF;
        return -errno;
    }
    void * helper = fuse_get_context()->private_data;
    memset(buf, 0, length);
    int name_length = strlen(filename);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = filename[i+1];
    }
    int size = file_size(name, helper);
    if (length > size) {
        length = size;
    }
    int ret = read_file(name, offset, length, (void *) buf, helper);
    if (ret == 3) {
        return -EIO;
    } else if (ret == 0) {
        return length;
    } else if (ret == 1) {
        return -errno;
    } else if (ret == 2) {
        return -errno;
    }
    return -errno;
}
    // FILL OUT

int myfuse_write(const char * filename, const char * buf, size_t length, off_t offset, struct fuse_file_info * fi) {
    if (fi->fh == 0) {
        return -EBADF;
    }
    void * helper = fuse_get_context()->private_data;
    int name_length = strlen(filename);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = filename[i+1];
    }
    ssize_t filesize = file_size(name, helper);
	if (filesize == -1) {
		errno = ENOENT;
		return -errno;
	}
    int ret = -1;
    if (filesize < length) {
        int counter = 0;
        while (counter < length) {
		char c = buf[counter];
		void * ptr = &c;
            ret = write_file(name, (off_t)counter + offset, 1, ptr, helper);
            if (ret == 2) {
                errno = EFBIG;
                return -errno;
            } else if (ret == 3) {
                return counter;
            }
            counter++;
        }
    } else {
        ret = write_file(name, offset, length, (void *) buf, helper);
    }
    if (ret == 0) {
        return length;
    } else if (ret == 2) {
	    errno = EFBIG;
    } else if (ret == 3) {
	    errno = ENOSPC;
    }
    return -errno;
}
    // FILL OUT

int myfuse_release(const char * filename, struct fuse_file_info * fi) {
    if (fi->fh == 1) {
        fi->fh = 0;
    } else {
        return -errno;
    }
    return -errno;
}
    // FILL OUT

void * myfuse_init(struct fuse_conn_info * fi) {
    void * helper = init_fs(file_data_file_name, directory_table_file_name, hash_data_file_name, 0);
    return helper;
}
    // FILL OUT

void myfuse_destroy(void * helper) {
    close_fs(helper);
    return;
}
    // FILL OUT

int myfuse_create(const char * filename, mode_t mode, struct fuse_file_info * fi) {
    void * helper = fuse_get_context()->private_data;  
	int name_length = strlen(filename);
    char name[name_length];
    for (int i = 0; i < name_length; i++) {
        name[i] = filename[i+1];
    }
    int ret = create_file(name, 0, helper);
    if (ret == 2) {
        return -errno;
    } else if (ret == 1) {
        return -errno;
    } else if (ret == 0) {
	fi->fh = 1;
        return 0;
    }
    return -errno;
}
    // FILL OUT

struct fuse_operations operations = {
    .getattr = myfuse_getattr,
    .readdir = myfuse_readdir,
    .unlink = myfuse_unlink,
    .rename = myfuse_rename,
    .truncate = myfuse_truncate,
    .open = myfuse_open,
    .read = myfuse_read,
    .write = myfuse_write,
    .release = myfuse_release,
    .init = myfuse_init,
    .destroy = myfuse_destroy,
    .create = myfuse_create
};

int main(int argc, char * argv[]) {
    // MODIFY (OPTIONAL)
    if (argc >= 5) {
        if (strcmp(argv[argc-4], "--files") == 0) {
            file_data_file_name = argv[argc-3];
            directory_table_file_name = argv[argc-2];
            hash_data_file_name = argv[argc-1];
            argc -= 4;
        }
    }
    // After this point, you have access to file_data_file_name, directory_table_file_name and hash_data_file_name
    int ret = fuse_main(argc, argv, &operations, NULL);
    return ret;
}
