#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define TEST(x) test(x, #x)
#include "myfilesystem.h"
#include "nodes.h"

/* You are free to modify any part of this file. The only requirement is that when it is run, all your tests are automatically executed */

/* Some example unit test functions */
void readfiles(char* filename) {
    //struct helper_d * help = (struct helper_d *) helper;
    struct file_d file;
    FILE* directory = fopen(filename, "rb");
    fseek(directory, 0, SEEK_END);
    int MAX = ftell(directory);
    fseek(directory, 0, SEEK_SET);
    int count = 0;
    printf("%s SIZE: %d\n", filename, MAX);
	while (ftell(directory) != MAX) {
		fread(&file, sizeof(struct file_d), 1, directory);
		if (*(file.filename) == '\0') {
            printf("empty slot\n");
            continue;
		}
        printf("FILE %d: %s\n", count, file.filename);
        printf("LENGTH: %d OFFSET: %d\n", file.length, file.offset);
		printf("DIRECTORY OFFSET: %d\n", count * 72);
        printf("\n");
        count++;
    }
}

void get_file_size(char* filename) {
    FILE* file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    printf("%s size is %ld\n", filename, ftell(file));
    fclose(file);
    return;
}

int compare_before_after(char* before_file, char* after_file) {
    FILE* before_directory = fopen(before_file, "rb");
    fseek(before_directory, 0, SEEK_END);
    FILE* after_directory = fopen(after_file, "rb");
    fseek(after_directory, 0, SEEK_END);
    if (ftell(before_directory) != ftell(after_directory)) {
        printf("%s not correct: ", after_file); 
        printf("files not of same size\n");
        printf("expected file of %ld total bytes\n", ftell(before_directory));
        printf("got file of %ld total bytes\n", ftell(after_directory));
        fclose(before_directory);
        fclose(after_directory);
        return 1;
    } else {
        char * before_buf = malloc(ftell(before_directory));
        char * after_buf = malloc(ftell(after_directory));
        fseek(before_directory, 0, SEEK_SET);
        fseek(after_directory, 0, SEEK_SET);
        fread(before_buf, sizeof(before_buf), 1, before_directory);
        fread(after_buf, sizeof(after_buf), 1, after_directory);
        int result = memcmp(before_buf, after_buf, sizeof(ftell(after_directory)));
        //printf("content: %s\n", before_buf);
        if (result != 0) {
            printf("%s not correct: ", after_file);  
            printf("byte %d in received output is %p and expected output is %p\n", result, before_buf + result, after_buf + result);
            fclose(before_directory);
            fclose(after_directory);
            free(after_buf);
            free(before_buf);
            return 1;
        }
        free(before_buf);
        free(after_buf);
    }
    fclose(before_directory);
    fclose(after_directory);
    return 0;
}

int success() {
    return 0;
}

int failure() {
    return 1;
}

int no_operation() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    close_fs(helper);
    return 0;
}

int create_existing_file() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    int ret = create_file("hello", 20, helper);
    close_fs(helper);
    if (ret == 1) {
        return compare_before_after("testcases/directory_table_0.bin", "directory_table.bin");
    }
    return ret;
}

int create_new_file_simple() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    int ret = create_file("new_file", 20, helper);
    close_fs(helper);
    if (ret == 0) {
        return compare_before_after("testcases/directory_table_0.bin", "directory_table.bin");
    }
    return ret;
}

int create_file_data_no_space() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    int ret = create_file("bigfile", 10000, helper);
	close_fs(helper);
    if (ret == 2) {
        return compare_before_after("testcases/directory_table_0.bin", "directory_table.bin");
    }
    return ret;
}

int create_file_directory_no_space() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    //readfiles("2_directory_table.bin");
    int ret = create_file("bigfile", 5, helper);
	close_fs(helper);
    if (ret == 2) {
        return compare_before_after("testcases/directory_table_2.bin", "2_directory_table.bin");
    }
    return ret;
}

int delete_file_success() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    int ret = delete_file("new_file", helper);
    close_fs(helper);
    if (ret == 0) {
        return compare_before_after("testcases/directory_table_3.bin", "directory_table.bin");
    }
    return ret;
}

int delete_file_fail() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    int ret = delete_file("nonexistingfile", helper);
    close_fs(helper);
    if (ret == 1) {
        return compare_before_after("testcases/directory_table_3.bin", "directory_table.bin");
    }
    return ret;
}

int rename_file_success() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    int ret = rename_file("1.docx", "new_name", helper);
    close_fs(helper);
    if (ret == 0) {
        return compare_before_after("testcases/directory_table_4.bin", "2_directory_table.bin");
    }
    return ret;
}

int rename_file_fail() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    int ret = rename_file("doesnotexist", "test", helper);
    close_fs(helper);
    if (ret == 1) {
        return compare_before_after("testcases/directory_table_3.bin", "directory_table.bin");
    }
    return ret;
}

int rename_file_exists() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    int ret = rename_file("new_name", "2.docx", helper);
    close_fs(helper);
    if (ret == 1) {
        return compare_before_after("testcases/directory_table_4.bin", "2_directory_table.bin");
    }
    return ret;
}

int resize_file_simple() {
    void * helper = init_fs("resize_file_data.bin", "resize_directory_table.bin", "resize_hash_data.bin", 1);
    int ret = resize_file("b.docx", 500, helper);
    close_fs(helper);
    if (ret == 0) {
        return compare_before_after("testcases/directory_table_resize.bin", "resize_directory_table.bin");
    }
    return ret;
}

int resize_file_not_exist() {
    void * helper = init_fs("resize_file_data.bin", "resize_directory_table.bin", "resize_hash_data.bin", 1);
    int ret = resize_file("doesnotexist", 10, helper);
    close_fs(helper);
    if (ret == 1) {
        return compare_before_after("testcases/directory_table_resize.bin", "resize_directory_table.bin");
    }
    return ret;
}

int resize_file_no_space() {
    void * helper = init_fs("resize_file_data.bin", "resize_directory_table.bin", "resize_hash_data.bin", 1);
    int ret = resize_file("a.docx", 600, helper);
    close_fs(helper);
    if (ret == 2) {
        return compare_before_after("testcases/directory_table_resize.bin", "resize_directory_table.bin");
    }
    return ret;
}

int write_data_simple() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    char * input = "123456789";
    void * buffer = (void *) input;
    int ret = write_file("Document.docx", 0, strlen(input), buffer, helper);
    close_fs(helper);
    if (ret == 0) {
        int ret_1 = compare_before_after("testcases/directory_table_3.bin", "directory_table.bin");
        int ret_2 = compare_before_after("testcases/file_data_write.bin", "file_data.bin");
        int ret_3 = compare_before_after("testcases/hash_data_write.bin", "hash_data.bin");
        return (ret_1 || ret_2) || ret_3;
    }
    return ret;
}

int write_data_resize() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    char * input = "writing hashing testcases is very difficult................";
    void * buffer = (void *) input;
    int ret = write_file("3.docx", 5, strlen(input), buffer, helper);
    close_fs(helper);
    if (ret == 0) {
        int ret_1 = compare_before_after("testcases/directory_table_4.bin", "2_directory_table.bin");
        int ret_2 = compare_before_after("testcases/file_data_write1.bin", "2_file_data.bin");
        int ret_3 = compare_before_after("testcases/hash_data_write1.bin", "2_hash_data.bin");
        return (ret_1 || ret_2) || ret_3;
    }
    return ret;
}

int write_data_no_space() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    char input[500] = {'o'};
    int ret = write_file("3.docx", 19, 500, input, helper);
    close_fs(helper);
    if (ret == 3) {
        int ret_1 = compare_before_after("testcases/directory_table_4.bin", "2_directory_table.bin");
        int ret_2 = compare_before_after("testcases/file_data_write1.bin", "2_file_data.bin");
        return ret_1 || ret_2;
    }
    return 0;
}

int write_data_fail() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    char * input = "writing hashing testcases is very difficult................";
    void * buffer = (void *) input;
    int ret = write_file("2.docx", 25, strlen(input), buffer, helper);
    close_fs(helper);
    if (ret == 2) {
        return 0;
    }
    return -1;
}

int repack_data() {
	void * helper = init_fs("repack_file_data.bin", "repack_directory_table.bin", "repack_hash_data.bin", 1);
	repack(helper);
    close_fs(helper);
    int ret_1 = compare_before_after("testcases/directory_table_repack.bin", "repack_directory_table.bin");
    int ret_2 = compare_before_after("testcases/file_data_repack.bin", "repack_file_data.bin");
    int ret_3 = compare_before_after("testcases/hash_data_repack.bin", "repack_hash_data.bin");
	return (ret_1 || ret_2) || ret_3;
}

int create_file_repack() {
    void * helper = init_fs("repack1_file_data.bin", "repack1_directory_table.bin", "repack1_hash_data.bin", 1);
    int ret = create_file("hooves", 100, helper);
    close_fs(helper);
    if (ret == 0) {
        int ret_1 = compare_before_after("testcases/directory_table_repack1.bin", "repack1_directory_table.bin");
        int ret_2 = compare_before_after("testcases/file_data_repack1.bin", "repack1_file_data.bin");
        int ret_3 = compare_before_after("testcases/hash_data_repack1.bin", "repack1_hash_data.bin");
        return (ret_1 || ret_2) || ret_3;
    }
    return ret;
}

int resize_file_repack() {
    void * helper = init_fs("resize_file_data.bin", "resize_directory_table.bin", "resize_hash_data.bin", 1);
    int ret = resize_file("a.docx", 524, helper);
    close_fs(helper);
    if (ret == 0) {
        int ret_1 = compare_before_after("testcases/directory_table_resize1.bin", "resize_directory_table.bin");
        int ret_2 = compare_before_after("testcases/file_data_resize1.bin", "resize_file_data.bin");
        int ret_3 = compare_before_after("testcases/hash_data_resize1.bin", "resize_hash_data.bin");
        return (ret_1 || ret_2) || ret_3;
    }
    return ret;
}

int write_file_repack() {
    void * helper = init_fs("repack2_file_data.bin", "repack2_directory_table.bin", "repack2_hash_data.bin", 1);
    char * input = "they have four legs.";
    void * buffer = (void *) input;
    int ret = write_file("unicorns", 0, strlen(input), buffer, helper);
    close_fs(helper);
    if (ret == 0) {
        int ret_1 = compare_before_after("testcases/directory_table_repack2.bin", "repack2_directory_table.bin");
        int ret_2 = compare_before_after("testcases/file_data_repack2.bin", "repack2_file_data.bin");
        int ret_3 = compare_before_after("testcases/hash_data_repack2.bin", "repack2_hash_data.bin");
        return (ret_1 || ret_2) || ret_3;
    }
    return ret;
}

int read_data_wrong_hash() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    void * buffer = malloc(sizeof(char) * 9);
    int ret = read_file("Document.docx", 0, 9, buffer, helper);
    close_fs(helper);
    if (ret == 3) {
        free(buffer);
        return 0;
    }
    free(buffer);
    return ret;
}

int read_data_fail() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    void * buffer = malloc(sizeof(char) * 20);
    int ret = read_file("new_name", 54, 20, buffer, helper);
    close_fs(helper);
    free(buffer);
    if (ret == 2) {
        return 0;
    }
    return ret;
}

int read_data_no_file() {
    void * helper = init_fs("2_file_data.bin", "2_directory_table.bin", "2_hash_data.bin", 1);
    void * buffer = malloc(sizeof(char) * 20);
    int ret = read_file("doesnotexist", 0, 20, buffer, helper);
    close_fs(helper);
    free(buffer);
    if (ret == 1) {
        return 0;
    }
    return ret;
}

int read_data_success() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    compute_hash_tree(helper);
    char * input = "123456789";
    void * buffer = malloc(sizeof(char) * 9);
    int ret = read_file("Document.docx", 0, 9, buffer, helper);
    close_fs(helper);
    if (ret == 0) {
        char * output = (char *) buffer;
        ret = memcmp(output, input, 9);
        free(buffer);
        return ret;
    }
    free(buffer);
    return 0;
}

int file_size_success() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    int ret = file_size("Document.docx", helper);
    close_fs(helper);
    if (ret == 59) {
        return 0;
    }
    return ret;
}

int file_size_fail() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    //readfiles(helper);
    int ret = file_size("doesnotexist", helper);
    close_fs(helper);
    if (ret == -1) {
        return 0;
    }
    return ret;
}

int compute_hash_tree_test() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    compute_hash_tree(helper);
    close_fs(helper);
    int ret = compare_before_after("testcases/hash_tree_testcase", "hash_data.bin");
    return ret;
}

int compute_hash_block_test() {
    void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 1);
    char * input = "new test to compute hash block";
    void * buffer = (void *) input;
    write_file("hello", 7, strlen(input), buffer, helper);
    for (int i = 0; i < 8; i++) {
        compute_hash_block(i, helper);
    }
    close_fs(helper);
    int ret = compare_before_after("testcases/hash_block_testcase", "hash_data.bin");
    return ret;
}

/****************************/

/* Helper function */
void test(int (*test_function) (), char * function_name) {
    int ret = test_function();
    if (ret == 0) {
        printf("Passed %s\n", function_name);
    } else {
        printf("Failed %s returned %d\n", function_name, ret);
    }
}
/************************/

int main(int argc, char * argv[]) {
    // You can use the TEST macro as TEST(x) to run a test function named "x"
    //TEST(success);
    //TEST(failure);
    
	printf("CREATE FILE TESTS\n");
	TEST(create_new_file_simple);            //done
    TEST(create_existing_file);              //done
	TEST(create_file_data_no_space);         //done
    TEST(create_file_directory_no_space);    //done
	printf("\n");
	
	printf("DELETE FILE TESTS\n");
    TEST(delete_file_success);               //done
    TEST(delete_file_fail);                  //done
	printf("\n");
	
	printf("RENAME FILE TESTS\n");
    TEST(rename_file_fail);                  //done
    TEST(rename_file_success);               //done
    TEST(rename_file_exists);                //done
	printf("\n");
	
	printf("RESIZE FILE TESTS\n");
    TEST(resize_file_simple);                //done
    TEST(resize_file_not_exist);             //done
    TEST(resize_file_no_space);              //done
	printf("\n");
    
    printf("WRITE DATA TESTS\n");
    TEST(write_data_simple);                 //done
    TEST(write_data_resize);                 //done
    TEST(write_data_no_space);               //done
    TEST(write_data_fail);                   //done
	printf("\n");
    
    printf("READ DATA TESTS\n");
    TEST(read_data_no_file);                 //done
    TEST(read_data_fail);                    //done
    TEST(read_data_wrong_hash);              //done
    TEST(read_data_success);                 //done
    printf("\n");
	
	printf("REPACK DATA TESTS\n");
	TEST(repack_data);                       //done
    TEST(create_file_repack);                //error
    TEST(resize_file_repack);                //done
    TEST(write_file_repack);                 //error
	printf("\n");
	
	printf("FILE SIZE TESTS\n");
	TEST(file_size_success);                 //done
	TEST(file_size_fail);                    //done
	printf("\n");
    
    printf("COMPUTE HASH TREE/BLOCKS TESTS\n");
    TEST(compute_hash_tree_test);            //done
    TEST(compute_hash_block_test);           //done
    
    
    return 0;
}
