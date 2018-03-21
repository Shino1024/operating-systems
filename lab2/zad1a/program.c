#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>

#define NUM_OF_COMMANDS 3

#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof(*(x)))

int error_code;

typedef enum operation {
	GENERATE = 0,
	SORT,
	COPY,
	UNDEFINED
} operation;

typedef enum mode {
	SYS,
	LIB,
	UNDEFINED_MODE
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
	char **args;
} command;

typedef unsigned char * record;

typedef unsigned char ** dataset;

int gen_record(record dest, unsigned int size) {
	if (dest == NULL) {
		return -1;
	}

	struct timeval rand_tv;
	gettimeofday(&rand_tv, NULL);
	srand48(rand_tv.tv_usec + (rand_tv.tv_sec % 1000) * 1E6);
	
	unsigned int i;
	for (i = 0; i < size; ++i) {
		dest[i] = (unsigned char) (lrand48() % 94 + 33);
	}

	return 0;
}

int get_nth_record_lib(FILE *file, unsigned int i, const record record_buffer, unsigned int record_size) {
	if (file == NULL) {
		return -1;
	}

	error_code = fseek(file, record_size * i, 0);
	if (error_code != 0) {
		return -2;
	}

	error_code = fread(record_buffer, sizeof(char), record_size, file);
	if (error_code != record_size) {
		return -3;
	}

	error_code = fseek(file, 0, 0);
	if (error_code != 0) {
		return -4;
	}

	return 0;
}

int set_nth_record_lib(FILE *file, unsigned int i, const record record_buffer, unsigned int record_size) {
	if (file == NULL) {
		return -1;
	}

	error_code = fseek(file, record_size * i, 0);
	if (error_code != 0) {
		return -2;
	}

	error_code = fwrite(record_buffer, sizeof(char), record_size, file);
	if (error_code < record_size) {
		return -3;
	}

	error_code = fseek(file, 0, 0);
	if (error_code != 0) {
		return -4;
	}

	return 0;
}

int get_nth_record_sys(int fd, unsigned int i, const record record_buffer, unsigned int record_size) {
	if (fd < 0) {
		return -1;
	}

	error_code = lseek(fd, record_size * i, SEEK_SET);
	if (error_code < 0) {
		return -2;
	}

	error_code = read(fd, record_buffer, record_size);
	if (error_code < 1) {
		return -3;
	}

	error_code = lseek(fd, 0, SEEK_SET);
	if (error_code < 0) {
		return -4;
	}

	return 0;
}

int set_nth_record_sys(int fd, unsigned int i, const record record_buffer, unsigned int record_size) {
	if (fd < 0) {
		return -1;
	}

	error_code = lseek(fd, record_size * i, SEEK_SET);
	if (error_code < 0) {
		return -2;
	}

	error_code = write(fd, record_buffer, record_size);
	if (error_code < 1) {
		return -3;
	}

	error_code = lseek(fd, 0, SEEK_SET);
	if (error_code < 0) {
		return -4;
	}

	return 0;
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

int check_filename(const char *filename) {
	if (filename == NULL) {
		return -1;
	}

	unsigned int i;
	unsigned int filename_len = strlen(filename);
	for (i = 0; i < filename_len; ++i) {
		if (filename[i] == '/') {
			printf("%s is not a correct filename.\n", filename);
			return -2;
		}
	}

	return 0;
}

int check_existing_filename(const char *filename) {
	error_code = check_filename(filename);
	if (error_code < 0) {
		return -1;
	}
	
	struct stat file_stat;
	error_code = lstat(filename, &file_stat);
	if (error_code != 0) {
		printf("%s does not exist.\n", filename);
		return -3;
	}

	if (S_ISREG(file_stat.st_mode) == 0) {
		printf("%s is not a regular file.\n", filename);
		return -4;
	}

	return 0;
}

int check_unsigned_int(const char *unsigned_int) {
	if (unsigned_int == NULL) {
		return -1;
	}

	unsigned int dummy;
	if (sscanf(unsigned_int, "%u", &dummy) == 0) {
		printf("%s is not a positive integer.\n", unsigned_int);
		return -2;
	}

	return 0;
}

int check_mode(const char *m) {
	if (m == NULL) {
		return -1;
	}

	unsigned int i;
	for (i = 0; i < SIZEOF_ARRAY(mode_names); ++i) {
		if (strcmp(mode_names[i], m) == 0) {
			return 0;
		}
	}

	printf("%s is not a correct mode name.\n", m);
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

	typedef int (*check_func)(const char *);
	check_func *functions = (check_func *) calloc(arg_counts[com->op], sizeof(check_func));
	if (functions == NULL) {
		return -4;
	}

	switch (com->op) {
		case GENERATE:
			functions[0] = &check_filename;
			functions[1] = &check_unsigned_int;
			functions[2] = &check_unsigned_int;
			break;

		case SORT:
			functions[0] = &check_existing_filename;
			functions[1] = &check_unsigned_int;
			functions[2] = &check_unsigned_int;
			functions[3] = &check_mode;
			break;

		case COPY:
			functions[0] = &check_existing_filename;
			functions[1] = &check_filename;
			functions[2] = &check_unsigned_int;
			functions[3] = &check_unsigned_int;
			functions[4] = &check_mode;
			break;

		default:
			free(functions);
			return -5;
	}

	for (i = 0; i < arg_counts[com->op]; ++i) {
		error_code = functions[i]((const char *) com->args[i]);
	}
	if (error_code < 0) {
		return -6;
	}

	free(functions);

	return 0;
}

int generate_records(const char *filename, unsigned int records, unsigned int record_len) {
	if (filename == NULL) {
		return -1;
	}

	FILE *out_file = fopen(filename, "w");
	if (out_file == NULL) {
		return -2;
	}

	record record_buffer = (record) calloc(record_len, sizeof(char));
	if (record_buffer == NULL) {
		fclose(out_file);
		return -3;
	}

	unsigned int i;
	for (i = 0; i < records; ++i) {
		error_code = gen_record(record_buffer, record_len);
		if (error_code < 0) {
			fclose(out_file);
			free(record_buffer);
			return -4;
		}

		error_code = fwrite(record_buffer, sizeof(char), record_len, out_file);
		if (error_code != record_len) {
			fclose(out_file);
			free(record_buffer);
			return -5;
		}
	}

	free(record_buffer);
	fclose(out_file);

	return 0;
}

int sort_records_lib(const char *filename, unsigned int records, unsigned int record_len) {
	if (filename == NULL) {
		return -1;
	}

	record r0 = (record) calloc(record_len, sizeof(char));
	if (r0 == NULL) {
		return -2;
	}
	record r1 = (record) calloc(record_len, sizeof(char));
	if (r1 == NULL) {
		free(r0);
		return -2;
	}

	FILE *out_file = fopen(filename, "r+");
	if (out_file == NULL) {
		free(r0);
		free(r1);
		return -3;
	}

	unsigned int i, j;
	for (i = 0; i < records; ++i) {
		error_code = get_nth_record_lib(out_file, i, r1, record_len);
		if (error_code < 0) {
			free(r0);
			free(r1);
			fclose(out_file);
			return -4;
		}
		for (j = 0; j < i; ++j) {
			error_code = get_nth_record_lib(out_file, j, r0, record_len);
			if (error_code < 0) {
				free(r0);
				free(r1);
				fclose(out_file);
				return -4;
			}
			if (r0[0] > r1[0]) {
				unsigned int k;
				for (k = i; k > j; --k) {
					error_code = get_nth_record_lib(out_file, k - 1, r0, record_len);
					error_code = get_nth_record_lib(out_file, k, r1, record_len);
					error_code = set_nth_record_lib(out_file, k - 1, r1, record_len);
					error_code = set_nth_record_lib(out_file, k, r0, record_len);
					if (error_code < 0) {
						free(r0);
						free(r1);
						fclose(out_file);
						return -4;
					}
				}
				break;
			}
		}
	}

	free(r0);
	free(r1);
	fclose(out_file);

	return 0;
}

int sort_records_sys(const char *filename, unsigned int records, unsigned int record_len) {
	if (filename == NULL) {
		return -1;
	}

	record r0 = (record) calloc(record_len, sizeof(char));
	if (r0 == NULL) {
		return -2;
	}
	record r1 = (record) calloc(record_len, sizeof(char));
	if (r1 == NULL) {
		free(r0);
		return -2;
	}

	int out_fd = open(filename, O_RDWR);
	if (out_fd < 0) {
		free(r0);
		free(r1);
		return -2;
	}

	unsigned int i, j;
	for (i = 0; i < records; ++i) {
		error_code = get_nth_record_sys(out_fd, i, r1, record_len);
		if (error_code < 0) {
			free(r0);
			free(r1);
			close(out_fd);
			return -4;
		}
		for (j = 0; j < i; ++j) {
			error_code = get_nth_record_sys(out_fd, j, r0, record_len);
			if (error_code < 0) {
				free(r0);
				free(r1);
				close(out_fd);
				return -4;
			}
			if (r0[0] > r1[0]) {
				unsigned int k;
				for (k = i; k > j; --k) {
					error_code = get_nth_record_sys(out_fd, k - 1, r0, record_len);
					error_code = get_nth_record_sys(out_fd, k, r1, record_len);
					error_code = set_nth_record_sys(out_fd, k - 1, r1, record_len);
					error_code = set_nth_record_sys(out_fd, k, r0, record_len);
					if (error_code < 0) {
						free(r0);
						free(r1);
						close(out_fd);
						return -4;
					}
				}
				break;
			}
		}
	}

	free(r0);
	free(r1);
	close(out_fd);

	return 0;
}

int sort_records(const char *filename, unsigned int records, unsigned int record_len, mode m) {
	if (filename == NULL) {
		return -1;
	}

	switch (m) {
		case SYS:
			error_code = sort_records_sys(filename, records, record_len);
			if (error_code < 0) {
				return -2;
			}
			break;

		case LIB:
			error_code = sort_records_lib(filename, records, record_len);
			if (error_code < 0) {
				return -2;
			}
			break;

		default:
			return -3;
	}
	
	return 0;
}

int copy_file_sys(const char *src, const char *dest, unsigned int records, unsigned int record_len) {
	int fd_src = open(src, O_RDONLY);
	if (fd_src < 0) {
		return -1;
	}

	long read_bytes = lseek(fd_src, 0, SEEK_END);
	if (read_bytes < records * record_len) {
		close(fd_src);
		return -2;
	} else {
		lseek(fd_src, 0, SEEK_SET);
	}

	int fd_dest = open(dest, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd_dest < 0) {
		return -3;
	}

	record record_buffer = (record) calloc(record_len, sizeof(char));
	if (record_buffer == NULL) {
		close(fd_src);
		close(fd_dest);
		return -4;
	}

	unsigned int i;
	for (i = 0; i < records; ++i) {
		error_code = read(fd_src, record_buffer, record_len);
		if (error_code < 1) {
			close(fd_src);
			close(fd_dest);
			free(record_buffer);
			return -3;
		}
		error_code = write(fd_dest, record_buffer, record_len);
		if (error_code < 1) {
			close(fd_src);
			close(fd_dest);
			free(record_buffer);
			return -3;
		}
	}

	close(fd_src);
	close(fd_dest);
	free(record_buffer);

	return 0;
}

int copy_file_lib(const char *src, const char *dest, unsigned int records, unsigned int record_len) {
	FILE *out_src = fopen(src, "r");
	if (out_src == NULL) {
		return -1;
	}

	error_code = fseek(out_src, 0, 2);
	if (error_code != 0) {
		fclose(out_src);
		return -2;
	}

	long read_bytes = ftell(out_src);
	if (read_bytes < records * record_len) {
		fclose(out_src);
		return -3;
	}

	error_code = fseek(out_src, 0, 0);
	if (error_code != 0) {
		fclose(out_src);
		return -4;
	}

	FILE *out_dest = fopen(dest, "w");
	if (out_dest == NULL) {
		fclose(out_src);
		return -5;
	}

	record record_buffer = (record) calloc(record_len, sizeof(char));
	if (record_buffer == NULL) {
		fclose(out_src);
		fclose(out_dest);
		return -6;
	}

	unsigned int i;
	for (i = 0; i < records; ++i) {
		error_code = fread(record_buffer, sizeof(char), record_len, out_src);
		if (error_code < 1) {
			fclose(out_src);
			fclose(out_dest);
			free(record_buffer);
			return -7;
		}
		error_code = fwrite(record_buffer, sizeof(char), record_len, out_dest);
		if (error_code < 1) {
			fclose(out_src);
			fclose(out_dest);
			free(record_buffer);
			return -8;
		}
	}

	fclose(out_src);
	fclose(out_dest);
	free(record_buffer);

	return 0;
}

int copy_file(const char *src, const char *dest, unsigned int records, unsigned int record_len, mode m) {
	if (src == NULL || dest == NULL) {
		return -1;
	}

	switch (m) {
		case SYS:
			error_code = copy_file_sys(src, dest, records, record_len);
			if (error_code < 0) {
				return -2;
			}
			break;

		case LIB:
			error_code = copy_file_lib(src, dest, records, record_len);
			if (error_code < 0) {
				return -2;
			}
			break;

		default:
			return -3;
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
	command *com = (command *) calloc(1, sizeof(command));
	if (com == NULL) {
		return NULL;
	}
	com->op = UNDEFINED;
	com->args = NULL;

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

char *parse_filename(char *filename) {
	if (filename == NULL) {
		return NULL;
	}

	return filename;
}

unsigned int parse_unsigned_int(const char *unsigned_int) {
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

	return UNDEFINED_MODE;
}

int perform_operation(command *com) {
	if (com == NULL) {
		return -1;
	}

	if (com->args == NULL) {
		return -2;
	}

	switch (com->op) {
		case GENERATE:
			{
				char *filename = parse_filename(com->args[0]);
				unsigned int records = parse_unsigned_int(com->args[1]);
				unsigned int record_size = parse_unsigned_int(com->args[2]);
				error_code = generate_records(filename, records, record_size);
				if (error_code < 0) {
					return -4;
				}
				break;
			}

		case SORT:
			{
				char *filename = parse_filename(com->args[0]);
				unsigned int records = parse_unsigned_int(com->args[1]);
				unsigned int record_size = parse_unsigned_int(com->args[2]);
				mode m = parse_mode(com->args[3]);
				error_code = sort_records(filename, records, record_size, m);
				if (error_code < 0) {
					return -4;
				}
				break;
			}

		case COPY:
			{
				char *src = parse_filename(com->args[0]);
				char *dest = parse_filename(com->args[1]);
				unsigned int records = parse_unsigned_int(com->args[2]);
				unsigned int record_size = parse_unsigned_int(com->args[3]);
				mode m = parse_mode(com->args[4]);
				error_code = copy_file(src, dest, records, record_size, m);
				if (error_code < 0) {
					return -4;
				}
				break;
			}

		default:
			return -5;
	}

	return 0;
}

int main(int argc, char *argv[]) {
	command *com = parse_args(argc, argv);
	if (com == NULL) {
		return 1;
	}

	error_code = perform_operation(com);
	if (error_code < 0) {
		printf("Errors occured. Exiting.\n");
		return 2;
	}

	free_command(com);

	return 0;
}
