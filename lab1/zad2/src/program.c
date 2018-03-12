#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "hdr/typedefs.h"

#include "hdr/array_dynamic.h"
#include "hdr/array_static.h"

#define COMMAND_NUMBER 3

#define REPEATS 10000

#define RESULT_FILENAME "raport2.txt"

typedef enum command_type {
	FIND = 'f',
	REMOVE_ADD = 'x',
	REMOVE_ADD_ALTERNATIVELY = 'y',
} command_type;

typedef struct command {
	command_type type;
	unsigned int arg;
} command;

typedef enum alloc_method {
	STATIC,
	DYNAMIC,
} alloc_method;

typedef struct test_result {
	struct timeval user;
	struct timeval sys;
	struct timeval real;
	char *result_info;
} test_result;

command command_array[COMMAND_NUMBER];
unsigned int command_iter;

unsigned int array_size;
unsigned int block_size;
alloc_method method = DYNAMIC;

void usage(char *program_name) {
	printf("Usage: %s\n\t-a <number_of_elements_in_array>\n\t-b <single_block_size>\n\t-s (if you want to use static, not dynamic arrays)\n\t-f <index_of_block_to_find_the_most_similar_block_to>\n\t-x <number_of_blocks_to_be_removed_and_created>\n\t-y <number_of_blocks_to_be_removed_and_created_alternatively>\n", program_name);
}

void parse_args(int argc, char *argv[]) {
	int flag;
	while ((flag = getopt(argc, argv, "ha:b:sf:x:y:")) != -1) {
		switch (flag) {
			case 'h':
				usage(argv[0]);
				exit(0);

			case 'a':
				array_size = (unsigned int) atoi(optarg);
				break;

			case 'b':
				block_size = (unsigned int) atoi(optarg);
				break;

			case 's':
				alloc_method = 's';
				break;

			case 'f':

			case 'x':

			case 'y':
				if (command_iter < MAX_COMMAND_NUMBER) {
					command new_command = { .type = flag, .arg = (unsigned int) atoi(optarg) };
					command_array[command_iter] = new_command;
					++command_iter;
				}
				break;

			default:
				break;
		}
	}
}

test_result perform_test() {
}

void append_result(char *result_info) {
}

int save_result(char *result_info) {
}

int main(int argc, char *argv[]) {
	parse_args(argc. argv);

	test_result runtime = perform_test();

	save_result(test_result->result_info);

	return 0;
}
