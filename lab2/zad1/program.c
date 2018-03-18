#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "util.h"
#include "parse.h"

#define NUM_OF_COMMANDS 3

#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof(*(x)))

typedef enum operation {
	GENERATE = 0,
	SORT,
	COPY,
	UNDEFINED
} operation;

typedef enum mode {
	SYS,
	LIB
} mode;

const char *op_names[] = {
	"generate",
	"sort",
	"copy"
};

const char *mode_names[] = {
	"sys",
	"lib"
};

const unsigned int arg_counts[] = {
	3,
	4,
	5
};

typedef struct command {
	operation op;
	char *args[];
} command;

typedef struct time_result {
	struct timeval real;
	struct timeval user;
	struct timeval sys;
} time_result;

struct timeval time_difference(struct timeval t0, struct timeval t1) {
}

void print_usage(char *program_name) {
	printf("Usage: %s"
			"\n\n\t(generate|sort|copy)"
			"\n\t(file|source destination)"
			"\n\tnumber_of_records"
			"\n\tlength_of_single_record"
			"\n\t(sys|lib) (in case of (sort|copy)"
			"\n", program_name);
}

typedef int (*check_func)(const char *);

int check_filepath(const char *filepath) {
	return 0;
}

int check_unsigned_int(const char *unsigned_int) {
	if (unsigned_int == NULL) {
		return NULL;
	}

	unsigned int dummy;
	if (sscanf(unsigned_int, "%u", &dummy) == 0) {
		return -2;
	}

	return 0;
}

int check_mode(const char *mode) {
	if (mode == NULL) {
		return -1;
	}

	unsigned int i;
	for (i = 0; i < SIZEOF_ARRAY(mode_names); ++i) {
		if (strcmp(mode_names[i], mode) == 0) {
			return 0;
		}
	}

	return -2;
}

int verify_command_args(command *com) {
	if (com == NULL) {
		return -1;
	}

	if (com->args == NULL) {
		return -2;
	}

	unsigned int i;
	for (i = 0; i < arg_counts[com->op]; ++i) {
		if (com->args[i] == NULL) {
			return -3;
		}
	}

	check_func *functions = (check_func *) calloc(arg_counts[com->op], sizeof(check_func));
	if (functions == NULL) {
		return -4;
	}

	switch (com->op) {
		case GENERATE:
			functions[0] = &check_filepath;
			functions[1] = &check_unsigned_int;
			functions[2] = &check_unsigned_int;

		case SORT:
			functions[0] = &check_filepath;
			functions[1] = &check_unsigned_int;
			functions[2] = &check_unsigned_int;
			functions[3] = &check_mode;
			break;

		case COPY:
			functions[0] = &check_filepath;
			functions[1] = &check_filepath;
			functions[2] = &check_unsigned_int;
			functions[3] = &check_unsigned_int;
			functions[4] = &check_mode;
			break;

		default:
			free(functions);
			return -5;
	}

	int error_code;
	for (i = 0; i < arg_counts[com->op]; ++i) {
		error_code = functions[i]((const char *) com->args[i]);
		if (error_code < 0) {
			return -6;
		}
	}

	free(functions);

	return 0;
}

int generate_record(const char *filename, unsigned int records, unsigned int record_len) {
	if (filename == NULL) {
		return -1;
	}

	FILE *out_file = fopen(filename, "w");
	if (out_file == NULL) {
		return -2;
	}

	record buffer = (record) calloc(record_len + 1, sizeof(*buffer));
	if (buffer == NULL) {
		fclose(out_file);
		return -3;
	}

	int error_code;
	unsigned int i;
	for (i = 0; i < records; ++i) {
		error_code = gen_record(record_buffer, record_len);
		record_buffer[record_len] = '\n';
		if (error_code < 0) {
			fclose(out_file);
			free(record_buffer);
			return -4;
		}

		error_code = fwrite(record_buffer, sizeof(record *), record_len + 1, out_file);
		if (error_code != record_len + 1) {
			fclose(out_file);
			free(record_buffer);
			return -5;
		}
	}

	free(record_buffer);
	fclose(out_file);

	return 0;
}

int sort_records(const char *filename, unsigned int records, unsigned int record_len, mode m) {
	if (filename == NULL) {
		return -1;
	}

	FILE *out_file = fopen(filename, "r");
	if (out_file == NULL) {
		return -2;
	}

	record r0 = (record) calloc(record_len, sizeof(record *));
	record r1 = (record) calloc(record_len, sizeof(record *));
	if (r0 == NULL) {
		if (r1 != NULL) {
			free(r1);
		}
		fclose(out_file);
		return -3;
	}
	if (r1 == NULL) {
		if (r0 != NULL) {
			free(r0);
		}
		fclose(out_file);
		return -3;
	}

	int error_code;
	unsigned int i, j;
	for (i = 0; i < records; ++i) {
		error_code = get_nth_record(out_file, i, r0, record_len);
		if (error_code < 0) {
			fclose(out_file)
			return -4;
		}
		for (j = 0; j < i; ++j) {
			error_code = get_nth_record(out_file, j, r0, record_len);
			if (error_code < 0) {
				fclose(out_file);
				return -4;
			}
			if (r0[0] > r1[0]) {
				unsigned int k;
				for (k = i - 1; k >= j; --k) {
					error_code = get_nth_record(out_file, k - 1, r0, record_len);
					error_code = get_nth_record(out_file, k, r1, record_len);
					error_code = set_nth_record(out_file, k - 1, r1, record_len);
					error_code = set_nth_record(out_file, k, r0, record_len);
					if (error_code < 0) {
						fclose(out_file);
						return -4;
					}
				}
				break;
			}
		}
	}

	fclose(out_file);

	return 0;
}

int copy_files(const char *src, const char *dest, unsigned int records, unsigned int record_len, mode m) {
	if (src == NULL || dest == NULL) {
		return -1;
	}

	/*
	FILE *out_f0 = fopen(src, "r+");
	if (out_f0 == NULL) {
		return -2;
	}

	FILE *out_f1 = fopen(dest, "w");
	if (out_f1 == NULL) {
		fclose(out_f0);
		return -3;
	}

	record record_buffer = (record) calloc(record_len, sizeof(record *));
	if (record_buffer == NULL) {
		fclose(out_f0);
		fclose(out_f1);
		return -4;
	}
	*/

	int error_code;
	switch (m) {
		case SYS:
			error_code = copy_files_sys(src, dest, records, record_len);
			if (error_code < 0) {
				return 
			break;

		case LIB:
			break;

		default:
			fclose(out_f0);
			fclose(out_f1);
			free(record_buffer);
			return -;
	}
	
	return 0;
}

int free_command(command *com) {
	if (com == NULL) {
		return -1;
	}

	if (com->args == NULL) {
		free(com);
		return 0;
	}

	unsigned int i;
	for (i = 0; i < arg_counts[com->op]; ++i) {
		if (com->args[i] != NULL) {
			free(com->args[i]);
		}
	}
	free(com->args);
	free(com);

	return 0;
}

command * parse_args(int argc, char *argv[]) {
	command *com = (command *) calloc(1, sizeof(*com));
	if (com == NULL) {
		return NULL;
	}
	com->op = UNDEFINED;
	com->args = NULL;

	// auto
	if (argc < 5 || argc > 7) {
		print_usage(argv[0]);
		free_command(com);
		return NULL;
	}

	unsigned int i;
	for (i = 0; i < SIZEOF_ARRAY(op_names); ++i) {
		if (strcmp(argv[1], op_names[i]) == 0) {
			com->op = (operation) i;
			break;
		}
	}
	if (com->op == UNDEFINED || argc - 2 != arg_counts[com->op]) {
		print_usage(argv[0]);
		free_command(com);
		return NULL;
	}

	int error_code;
	com->args = (char **) calloc(arg_counts[com->op], sizeof(*(com->args)));
	if (com->args == NULL) {
		free_command(com);
		return NULL;
	}
	for (i = 0; i < arg_counts[com->op]; ++i) {
		int arg_len = strlen(argv[i + 2]);
		com->args[i] = (char *) calloc(arg_len + 2, sizeof(**(com->args)));
		if (com->args[i] == NULL) {
			free_command(com);
			return NULL;
		}
		memcpy(com->args[i], argv[i + 2], arg_len);
		com->args[i][arg_len] = '\0';
	}
	
	error_code = verify_command_args(com);
	if (error_code < 0) {
		free_command(com);
		return NULL;
	}

	return com;
}

// TODO
char *parse_filepath(char *filepath) {
	if (filepath == NULL) {
		return NULL;
	}

	return filepath;
}

unsigned int parse_unsigned_int(char *unsigned_int) {
	unsigned int ret;
	sscanf(unsigned_int, "%u", &ret);

	return ret;
}

mode parse_mode(char *m) {
	unsigned int i;
	for (i = 0; i < SIZEOF_ARRAY(mode_names); ++i) {
		if (strcmp(m, mode_names[i]) == 0) {
			return (mode) i;
		}
	}
}

int perform_operation(command *com) {
	if (com == NULL) {
		return -1;
	}

	if (com->args == NULL) {
		return -2;
	}

	int error_code;
	switch (com->op) {
		case GENERATE:
			{
				char *filepath = parse_filepath(com->args[0]);
				unsigned int records = parse_unsigned_int(com->args[1]);
				unsigned int record_size = parse_unsigned_int(com->args[2]);
				error_code = generate_records(filepath, records, record_size);
				if (error_code < 0) {
					return -4;
				}
				break;
			}

		case SORT:
			{
				char *filepath = parse_filepath(com->args[0]);
				unsigned int records = parse_unsigned_int(com->args[1]);
				unsigned int record_size = parse_unsigned_int(com->args[2]);
				mode m = parse_mode(com->args[3]);
				error_code = sort_records(filepath, records, record_size, m);
				if (error_code < 0) {
					return -4;
				}
				break;
			}

		case COPY:
			{
				char *src = parse_filepath(com->args[0]);
				char *dest = parse_filepath(com->args[1]);
				unsigned int records = parse_unsigned_int(com->args[2]);
				unsigned int record_size = parse_unsigned_int(com->args[3]);
				mode m = parse_mode(com->args[4]);
				error_code = copy_files(filepath, records, record_size, m);
				if (error_code < 0) {
					return -4;
				}
				break;
			}

		default:
			break;
	}

	return 0;
}

int main(int argc, char *argv[]) {
	command *com = parse_args(argc, argv);
	if (com == NULL) {
		return 1;
	}

	int error_code;
	error_code = perform_operation(com);
	if (error_code < 0) {
		return 2;
	}

	free_command(com);

	return 0;
}
