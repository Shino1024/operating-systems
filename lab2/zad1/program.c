#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define NUM_OF_COMMANDS 3

#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof(*(x)))

typedef enum operation {
	GENERATE = 0,
	SORT,
	COPY,
	UNDEFINED
} operation;

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
	mode m;
} command;

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

int verify_args(command *com) {
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
	for (i = 0; i < arg_counts; ++i) {
		error_code = functions[i]((const char *) com->args[i]);
		if (error_code < 0) {
			return -6;
		}
	}

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
	
	error_code = verify_args(com);
	if (error_code < 0) {
		free_command(com);
		return NULL;
	}

	return com;
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

int perform_operation(command *com) {
	if (com == NULL) {
		return -1;
	}

	//

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

	return 0;
}
