#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "hdr/typedefs.h"

#include "hdr/array_dynamic.h"
#include "hdr/array_static.h"

#ifndef MAX_COMMAND_NUMBER
	#define MAX_COMMAND_NUMBER 3
#endif // MAX_COMMAND_NUMBER

#ifndef RESULT_FILENAME
	#define RESULT_FILENAME "raport2.txt"
#endif // RESULT_FILENAME

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

unsigned int trials = { 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7 };

typedef struct test_result {
	struct timeval user[sizeof(trials)/sizeof(*trials)];
	struct timeval sys[sizeof(trials)/sizeof(*trials)];
	struct timeval real[sizeof(trials)/sizeof(*trials)];
} test_result;

typedef void command_function_static(unsigned int arg);
typedef void command_function_dynamic(array_dynamic ad, unsigned int arg);

void find_static(unsigned int arg) {
	find_most_matching_block_static(arg);
}

void remove_add_static(unsigned int arg) {
	unsigned int index;
	for (index = 0; index < arg; ++index) {
		pop_block_static(index);
	}
	for (index = 0; index < arg; ++index) {
		append_block_static(index);
	}
}

void remove_add_alternatively_static(unsigned int arg) {
	unsigned int index;
	for (index = 0; index < arg; ++index) {
		pop_block_static(index);
		append_block_static(index);
	}
}

void find_dynamic(array_dynamic *ad, unsigned int arg) {
	find_most_matching_block_dynamic(ad, arg);
}

void remove_add_dynamic(array_dynamic *ad, unsigned int arg) {
	unsigned int index;
	for (index = 0; index < arg; ++index) {
		pop_block_dynamic(ad, arg);
	}
	for (index = 0; index < arg; ++index) {
		append_block_dynamic(ad, arg);
	}
}

void remove_add_alternatively_dynamic(array_dynamic *ad, unsigned int arg) {
	unsigned int index;
	for (index = 0; index < arg; ++index) {
		pop_block_dynamic(ad, arg);
		append_block_dynamic(ad, arg);
	}
}

command command_array[MAX_COMMAND_NUMBER];
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
					command_array[command_iter] = {
						.type = flag,
						.arg = (unsigned int) atoi(optarg)
					};
					++command_iter;
				}
				break;

			default:
				break;
		}
	}
}

test_result * perform_test() {
	test_result *tr = calloc(command_iter + 1, sizeof(test_result));

	struct timeval real_time_start, real_time_end;
	struct rusage previous_usage, present_usage;
	unsigned int test_index;
	for (test_index = 0; test_index < sizeof(trials) / sizeof(*trials); ++test_index) {
		unsigned int command_no, trial_no;
		for (command_no = 0; command_no < MAX_COMMAND_NUMBER; ++command_no) {
			getrusage(RUSAGE_SELF, &previous_usage);
			gettimeofday(&real_time_start, NULL);

			if (method == DYNAMIC) {
				command_function_dynamic function;
				switch (command_array[command_no].type) {
					case 'f':
						function = find_dynamic;
						break;

					case 'x':
						function = remove_add_dynamic;
						break;

					case 'y':
						function = remove_add_alternatively_dynamic;
						break;
				}
				for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
					function(command_array[command_no].arg);
				}
			} else if (method == STATIC) {
				command_function_static function;
				switch(command_array[command_no].type) {
					case 'f':
						function = find_static;
						break;

					case 'x':
						function = remove_add_static;
						break;

					case 'y':
						function = remove_add_alternatively_static;
						break;
				}
				for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
					function(command_array[command_no].arg);
				}
			}

			getrusage(RUSAGE_SELF, &present_usage);
			gettimeofday(&real_time_end, NULL);

			tr[command_no].user[test_index] = {
				.tv_sec = present_usage.ru_utime.tv_sec - previous_usage.ru_utime.tv_sec,
				.tv_usec = present_usage.ru_utime.tv_usec - previous_usage.ru_utime.tv_usec
			};
			tr[command_no].sys[test_index] = {
				.tv_sec = present_usage.ru_stime.tv_sec - previous_usage.ru_stime.tv_sec,
				.tv_usec = present_usage.ru_stime.tv_usec - previous_usage.ru_stime.tv_usec
			};
			tr[command_no].real[test_index] = {
				.tv_sec = real_time_end.tv_sec - real_time_start.tv_sec,
				.tv_usec = real_time_end.tv_usec - real_time_start.tv_usec
			};
		}
	}

	return tr;
}

int save_result(test_result *result) {
	return 0;
}

int main(int argc, char *argv[]) {
	parse_args(argc. argv);

	test_result runtime = perform_test();

	save_result(test_result->result_info);

	return 0;
}
