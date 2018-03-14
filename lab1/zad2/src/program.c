#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "hdr/typedefs.h"

#include "imp/util.c"
#include "imp/array_dynamic.c"
#include "imp/array_static.c"

#ifndef MAX_COMMAND_NUMBER
	#define MAX_COMMAND_NUMBER 3
#endif // MAX_COMMAND_NUMBER

typedef enum command_type {
	FIND = 'f',
	REMOVE_ADD = 'x',
	REMOVE_ADD_ALTERNATELY = 'y',
} command_type;

typedef struct command {
	command_type type;
	unsigned int arg;
} command;

typedef enum alloc_method {
	STATIC,
	DYNAMIC,
} alloc_method;

unsigned int trials[] = { 1E1, 1E2, 1E3, 1E4, 1E5, 1E6 };

typedef struct test_result {
	struct timeval user[sizeof(trials)/sizeof(*trials)];
	struct timeval sys[sizeof(trials)/sizeof(*trials)];
	struct timeval real[sizeof(trials)/sizeof(*trials)];
} test_result;

typedef void (*command_function_static)(unsigned int arg);
typedef void (*command_function_dynamic)(array_dynamic *ad, unsigned int arg);

void find_static(unsigned int arg) {
	find_most_matching_block_static(arg);
}

void remove_add_static(unsigned int arg) {
	unsigned int index;
	for (index = 0; index < arg; ++index) {
		pop_block_static(index);
	}
	for (index = 0; index < arg; ++index) {
		append_block_gen_static(index);
	}
}

void remove_add_alternatively_static(unsigned int arg) {
	unsigned int index;
	for (index = 0; index < arg; ++index) {
		pop_block_static(index);
		append_block_gen_static(index);
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
		append_block_gen_dynamic(ad, arg);
	}
}

void remove_add_alternately_dynamic(array_dynamic *ad, unsigned int arg) {
	unsigned int index;
	for (index = 0; index < arg; ++index) {
		pop_block_dynamic(ad, arg);
		append_block_gen_dynamic(ad, arg);
	}
}

command command_array[MAX_COMMAND_NUMBER];
unsigned int command_iter;

unsigned int array_size;
unsigned int block_size;
alloc_method method;

void usage(char *program_name) {
	printf("Usage: %s"
			"\n\t-a <number_of_elements_in_array>"
			"\n\t-b <single_block_size>"
			"\n\t-s (if you want to use static, not dynamic arrays)"
			"\n\t-f <index_of_block_to_find_the_most_similar_block_to>"
			"\n\t-x <number_of_blocks_to_be_removed_and_created>"
			"\n\t-y <number_of_blocks_to_be_removed_and_created_alternatively>\n", program_name);
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
				method = STATIC;
				break;

			case 'f':

			case 'x':

			case 'y':
				if (command_iter < MAX_COMMAND_NUMBER) {
					command new_command = {
						flag,
						(unsigned int) atoi(optarg)
					};
					command_array[command_iter] = new_command;
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

	// Handling array creation
	for (test_index = 0; test_index < sizeof(trials) / sizeof(*trials); ++test_index) {
		unsigned int trial_no;
		if (method == DYNAMIC) {
			array_dynamic *test_ads[trials[test_index]];

			getrusage(RUSAGE_SELF, &previous_usage);
			gettimeofday(&real_time_start, NULL);

			for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
				test_ads[trial_no] = make_array_dynamic(array_size, block_size);
				unsigned int block_no;
				for (block_no = 0; block_no < block_size; ++block_no) {
					append_block_gen_dynamic(test_ads[trial_no], block_no);
				}
			}

			getrusage(RUSAGE_SELF, &present_usage);
			gettimeofday(&real_time_end, NULL);

			struct timeval user_tv = {
				(time_t) (present_usage.ru_utime.tv_sec - previous_usage.ru_utime.tv_sec),
				(time_t) (present_usage.ru_utime.tv_usec - previous_usage.ru_utime.tv_usec)
			};
			tr[0].user[test_index] = user_tv;
			struct timeval sys_tv = {
				(time_t) (present_usage.ru_stime.tv_sec - previous_usage.ru_stime.tv_sec),
				(time_t) (present_usage.ru_stime.tv_usec - previous_usage.ru_stime.tv_usec)
			};
			tr[0].sys[test_index] = sys_tv;
			struct timeval real_tv = {
				(time_t) (real_time_end.tv_sec - real_time_start.tv_sec),
				(time_t) (real_time_end.tv_usec - real_time_start.tv_usec)
			};
			tr[0].real[test_index] = real_tv;

			for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
				free_array_dynamic(&(test_ads[trial_no]));
			}
		} else if (method == STATIC) {
			struct timeval user_tv = { 0, 0 };
			tr[0].user[test_index] = user_tv;
			struct timeval sys_tv = { 0, 0 };
			tr[0].sys[test_index] = sys_tv;
			struct timeval real_tv = { 0, 0 };
			tr[0].real[test_index] = real_tv;
		}
	}

	// Handling provided actions
	array_dynamic *ad = make_array_dynamic(array_size, block_size);
	make_static_array(array_size, block_size);
	unsigned int block_index;
	for (block_index = 0; block_index < array_size; ++block_index) {
		append_block_gen_dynamic(ad, block_index);
	}
	for (test_index = 0; test_index < sizeof(trials) / sizeof(*trials); ++test_index) {
		unsigned int command_no, trial_no;
		for (command_no = 0; command_no < MAX_COMMAND_NUMBER; ++command_no) {
			getrusage(RUSAGE_SELF, &previous_usage);
			gettimeofday(&real_time_start, NULL);

			if (method == DYNAMIC) {
				command_function_dynamic function;
				switch (command_array[command_no].type) {
					case FIND:
						function = &find_dynamic;
						break;

					case REMOVE_ADD:
						function = &remove_add_dynamic;
						break;

					case REMOVE_ADD_ALTERNATELY:
						function = &remove_add_alternately_dynamic;
						break;
				}
				for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
					function(ad, command_array[command_no].arg);
				}
			} else if (method == STATIC) {
				command_function_static function;
				switch(command_array[command_no].type) {
					case 'f':
						function = &find_static;
						break;

					case 'x':
						function = &remove_add_static;
						break;

					case 'y':
						function = &remove_add_alternatively_static;
						break;
				}
				for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
					function(command_array[command_no].arg);
				}
			}

			getrusage(RUSAGE_SELF, &present_usage);
			gettimeofday(&real_time_end, NULL);

			struct timeval user_tv = {
				(time_t) (present_usage.ru_utime.tv_sec - previous_usage.ru_utime.tv_sec),
				(time_t) (present_usage.ru_utime.tv_usec - previous_usage.ru_utime.tv_usec)
			};
			tr[command_no + 1].user[test_index] = user_tv;
			struct timeval sys_tv = {
				(time_t) (present_usage.ru_stime.tv_sec - previous_usage.ru_stime.tv_sec),
				(time_t) (present_usage.ru_stime.tv_usec - previous_usage.ru_stime.tv_usec)
			};
			tr[command_no + 1].sys[test_index] = sys_tv;
			struct timeval real_tv = {
				(time_t) (real_time_end.tv_sec - real_time_start.tv_sec),
				(time_t) (real_time_end.tv_usec - real_time_start.tv_usec)
			};
			tr[command_no + 1].real[test_index] = real_tv;
		}
	}

	return tr;
}

int print_test_result(test_result *result) {
	printf("\nTest results for the following parameters:"
			"\n%s array,"
			"\narray size of %d,"
			"\nblock size of %d",
			method == DYNAMIC ? "DYNAMIC" : "STATIC",
			array_size,
			block_size);

	unsigned int command_no, trial_no;
	
	printf("\n\n");
	printf("\n\tArray creation:");
	printf("\n\t%12s|%12s|%12s|%12s", "repeats ", "real ", "user ", "sys ");
	for (trial_no = 0; trial_no < sizeof(trials) / sizeof(*trials); ++trial_no) {
		printf("\n\t%-12d|%2ld.%-09ld|%2ld.%-09ld|%2ld.%-09ld",
					trials[trial_no],
					result[0].real[trial_no].tv_sec,
					result[0].real[trial_no].tv_usec,
					result[0].user[trial_no].tv_sec,
					result[0].user[trial_no].tv_usec,
					result[0].sys[trial_no].tv_sec,
					result[0].sys[trial_no].tv_usec);
	}

	printf("\n\n");
	for (command_no = 0; command_no < command_iter; ++command_no) {
		switch (command_array[command_no].type) {
			case FIND:
				printf("\n\n\n\tCommand: FIND");
				break;

			case REMOVE_ADD:
				printf("\n\n\n\tCommand: REMOVE_ADD");
				break;

			case REMOVE_ADD_ALTERNATELY:
				printf("\n\n\n\tCommand: REMOVE_ADD_ALTERNATELY");
				break;

			default:
				break;
		}

		printf("\n\t%12s|%12s|%12s|%12s", "repeats ", "real ", "user ", "sys ");
		for (trial_no = 0; trial_no < sizeof(trials) / sizeof(*trials); ++trial_no) {
			printf("\n\t%-12d|%2ld.%-09ld|%2ld.%-09ld|%2ld.%-09ld",
					trials[trial_no],
					result[command_no + 1].real[trial_no].tv_sec,
					result[command_no + 1].real[trial_no].tv_usec,
					result[command_no + 1].user[trial_no].tv_sec,
					result[command_no + 1].user[trial_no].tv_usec,
					result[command_no + 1].sys[trial_no].tv_sec,
					result[command_no + 1].sys[trial_no].tv_usec);
		}	
	}

	printf("\n\nThe test has been finished successfully.\n\n");

	return 0;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}
	method = DYNAMIC;
	parse_args(argc, argv);

	test_result *runtime = perform_test();

	print_test_result(runtime);

	free(runtime);

	return 0;
}
