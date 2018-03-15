#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "condensed_lib.h"

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

unsigned int trials[] = { 1E1, 1E2, 1E3, 1E4, 1E5 };

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

void remove_add_alternately_static(unsigned int arg) {
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
			"\n\t-a <number_of_elements_in_array> (required)"
			"\n\t-b <single_block_size> (required)"
			"\n\t-s (if you want to use static, not dynamic arrays)"
			"\n\t-f <index_of_block_to_find_the_most_similar_block_to>"
			"\n\t-x <number_of_blocks_to_be_removed_and_created>"
			"\n\t-y <number_of_blocks_to_be_removed_and_created_alternatively>\n", program_name);
}

int convert_arg(char *arg) {
	int ret;
	if (sscanf(arg, "%d", &ret) == EOF) {
		return -1;
	} else {
		return ret;
	}
}

int parse_args(int argc, char *argv[]) {
	int flag;
	int arg_get;
	while ((flag = getopt(argc, argv, "ha:b:sf:x:y:")) != -1) {
		switch (flag) {
			case 'h':
				usage(argv[0]);
				return -3;

			case 'a':
				arg_get = convert_arg(optarg);
				if (arg_get < 0) {
					return -1;
				}

				array_size = (unsigned int) arg_get;
				break;

			case 'b':
				arg_get = convert_arg(optarg);
				if (arg_get < 0) {
					return -1;
				}

				block_size = (unsigned int) arg_get;
				break;

			case 's':
				method = STATIC;
				break;

			case 'f':

			case 'x':

			case 'y':
				arg_get = convert_arg(optarg);
				if (arg_get < 0) {
					return -1;
				}

				if (command_iter < MAX_COMMAND_NUMBER) {
					command new_command = {
						flag,
						(unsigned int) arg_get
					};
					command_array[command_iter] = new_command;
					++command_iter;
				}
				break;

			default:
				break;
		}
	}

	if (array_size < 1 || block_size < 1) {
		return -2;
	}

	unsigned int check_index;
	for (check_index = 0; check_index < command_iter; ++check_index) {
		if (command_array[check_index].type == FIND) {
			if (command_array[check_index].arg >= array_size) {
				return -4;
			} 
		} else if (command_array[check_index].type == REMOVE_ADD
				|| command_array[check_index].type == REMOVE_ADD_ALTERNATELY) {
			if (command_array[check_index].arg >= block_size) {
				return -4;
			}
		}
	}

	return 0;
}

struct timeval time_difference(struct timeval present, struct timeval previous) {
	struct timeval tv_ret;
	if (present.tv_sec < previous.tv_sec
			|| (present.tv_sec == previous.tv_sec && present.tv_usec < previous.tv_usec)) {
		tv_ret.tv_sec = 0;
		tv_ret.tv_usec = 0;
	} else {
		if (present.tv_sec == previous.tv_sec) {
			tv_ret.tv_sec = 0;
			tv_ret.tv_usec = (size_t) (present.tv_usec - previous.tv_usec);
		} else {
			if (present.tv_usec >= previous.tv_usec) {
				tv_ret.tv_sec = (size_t) (present.tv_sec - previous.tv_sec);
				tv_ret.tv_usec = (size_t) (present.tv_usec - previous.tv_usec);
			} else {
				tv_ret.tv_sec = (size_t) (present.tv_sec - previous.tv_sec - 1);
				tv_ret.tv_usec = (size_t) (1000000 - (previous.tv_usec - present.tv_usec));
			}
		}
	}

	return tv_ret;
}

test_result * perform_test() {
	test_result *tr = calloc(command_iter + 1, sizeof(test_result));

	struct timeval real_time_start, real_time_end;
	struct rusage previous_usage, present_usage;
	unsigned int test_index;

	// Handling array creation
	for (test_index = 0; test_index < sizeof(trials) / sizeof(*trials); ++test_index) {
		unsigned int trial_no, block_no;
		printf("Array creation test no %d, repeats: %d\n", test_index, trials[test_index]);
		if (method == DYNAMIC) {
			array_dynamic *test_ads[trials[test_index]];

			getrusage(RUSAGE_SELF, &previous_usage);
			gettimeofday(&real_time_start, NULL);

			for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
				test_ads[trial_no] = make_array_dynamic(array_size, block_size);
				for (block_no = 0; block_no < block_size; ++block_no) {
					append_block_gen_dynamic(test_ads[trial_no], block_no);
				}
			}

			getrusage(RUSAGE_SELF, &present_usage);
			gettimeofday(&real_time_end, NULL);

			tr[0].user[test_index] = time_difference(present_usage.ru_utime, previous_usage.ru_utime);
			tr[0].sys[test_index] = time_difference(present_usage.ru_stime, previous_usage.ru_stime);	
			tr[0].real[test_index] = time_difference(real_time_end, real_time_start);

			for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
				free_array_dynamic(&(test_ads[trial_no]));
			}
		} else if (method == STATIC) {
			getrusage(RUSAGE_SELF, &previous_usage);
			gettimeofday(&real_time_start, NULL);

			for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
				for (block_no = 0; block_no < block_size; ++block_no) {
					append_block_gen_static(block_no);
				}
			}

			getrusage(RUSAGE_SELF, &present_usage);
			gettimeofday(&real_time_end, NULL);

			tr[0].user[test_index] = time_difference(present_usage.ru_utime, previous_usage.ru_utime);
			tr[0].sys[test_index] = time_difference(present_usage.ru_stime, previous_usage.ru_stime);	
			tr[0].real[test_index] = time_difference(real_time_end, real_time_start);
		}
	}

	// Handling provided actions
	array_dynamic *ad;
	unsigned int block_index;
	if (method == DYNAMIC) {
		ad = make_array_dynamic(array_size, block_size);
		for (block_index = 0; block_index < array_size; ++block_index) {
			append_block_gen_dynamic(ad, block_index);
		}
	} else if (method == STATIC) {
		make_static_array(array_size, block_size);
		for (block_index = 0; block_index < array_size; ++block_index) {
			append_block_gen_static(block_index);
		}
	}

	for (test_index = 0; test_index < sizeof(trials) / sizeof(*trials); ++test_index) {
		unsigned int command_no, trial_no;
		printf("Command test no %d, trials: %d\n", test_index, trials[test_index]);
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

					default:
						return NULL;
				}
				for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
					function(ad, command_array[command_no].arg);
				}
			} else if (method == STATIC) {
				command_function_static function;
				switch (command_array[command_no].type) {
					case FIND:
						function = &find_static;
						break;

					case REMOVE_ADD:
						function = &remove_add_static;
						break;

					case REMOVE_ADD_ALTERNATELY:
						function = &remove_add_alternately_static;
						break;

					default:
						return NULL;
				}
				for (trial_no = 0; trial_no < trials[test_index]; ++trial_no) {
					function(command_array[command_no].arg);
				}
			}

			getrusage(RUSAGE_SELF, &present_usage);
			gettimeofday(&real_time_end, NULL);

			tr[command_no + 1].user[test_index] = time_difference(present_usage.ru_utime, previous_usage.ru_utime);
			tr[command_no + 1].sys[test_index] = time_difference(present_usage.ru_stime, previous_usage.ru_stime);	
			tr[command_no + 1].real[test_index] = time_difference(real_time_end, real_time_start);
		}
	}

	if (method == DYNAMIC) {
		free_array_dynamic(&ad);
	} else if (method == STATIC) {
		zero_out_static_array();
	}

	return tr;
}

int print_test_result_and_free(test_result *result) {
	printf("\nTest results for the following parameters:"
			"\n%s array,"
			"\narray size of %d,"
			"\nblock size of %d",
			method == DYNAMIC ? "DYNAMIC" : "STATIC",
			array_size,
			block_size);

	unsigned int command_no, trial_no;
	
	printf("\n\n\tArray creation:");
	printf("\n\t%12s|%12s|%12s|%12s|", "repeats ", "real ", "user ", "sys ");
	for (trial_no = 0; trial_no < sizeof(trials) / sizeof(*trials); ++trial_no) {
		printf("\n\t%-12d|%2ld.%06ld   |%2ld.%06ld   |%2ld.%06ld   |",
					trials[trial_no],
					result[0].real[trial_no].tv_sec,
					result[0].real[trial_no].tv_usec,
					result[0].user[trial_no].tv_sec,
					result[0].user[trial_no].tv_usec,
					result[0].sys[trial_no].tv_sec,
					result[0].sys[trial_no].tv_usec);
	}

	for (command_no = 0; command_no < command_iter; ++command_no) {
		switch (command_array[command_no].type) {
			case FIND:
				printf("\n\n\tCommand: FIND");
				break;

			case REMOVE_ADD:
				printf("\n\n\tCommand: REMOVE_ADD");
				break;

			case REMOVE_ADD_ALTERNATELY:
				printf("\n\n\tCommand: REMOVE_ADD_ALTERNATELY");
				break;

			default:
				break;
		}

		printf("\n\t%12s|%12s|%12s|%12s|", "repeats ", "real ", "user ", "sys ");
		for (trial_no = 0; trial_no < sizeof(trials) / sizeof(*trials); ++trial_no) {
			printf("\n\t%-12d|%2ld.%06ld   |%2ld.%06ld   |%2ld.%06ld   |",
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

	free(result);

	return 0;
}

int main(int argc, char *argv[]) {
	int error_code;
	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}
	method = DYNAMIC;
	
	error_code = parse_args(argc, argv);
	if (error_code == -1) {
		printf("Make sure the arguments to the provided options are positive numbers. Exiting.\n");
		return 1;
	} else if (error_code == -2) {
		printf("Provide array size and block size.\n");
		return 2;
	} else if (error_code == -3) {
		usage(argv[0]);
		return 0;
	} else if (error_code == -4) {
		printf("Make sure your test arguments fit given bounds (array size and block size)\n");
		return 3;
	}

	test_result *runtime = perform_test();
	if (runtime == NULL) {
		return 4;
	}

	print_test_result_and_free(runtime);

	return 0;
}
