#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/resource.h>

int error_code;

typedef struct command_bundle {
	char *program_name;
	char **arguments;
	int argument_count;
} command_bundle;

typedef struct time_stat {
	struct timeval user;
	struct timeval sys;
} time_stat;

typedef struct limited_batch {
	struct rlimit limit_as;
	struct rlimit limit_cpu;
	char *batch_name;
} limited_batch;

int print_usage(char *program_name) {
	printf("Usage: %s\n"
			"\n\tsource"
			"\n", program_name);

	return 0;
}

limited_batch * parse_args(int argc, char *argv[]) {
	if (argc != 4) {
		print_usage(argv[0]);
		return NULL;
	}

	limited_batch *ret = (limited_batch *) calloc(1, sizeof(limited_batch));
	if (ret == NULL) {
		return NULL;
	}

	unsigned long parsee;
	parsee = strtoul(argv[1], NULL, 10);
	if (parsee == 0
			|| parsee >= (ULONG_MAX << 11)) {
		printf("Provide valid positive unsigned long integers.\n");
		free(ret->batch_name);
		free(ret);
		return NULL;
	}
	ret->limit_as.rlim_cur = parsee << 20;
	ret->limit_as.rlim_max = ret->limit_as.rlim_cur;

	ret->limit_cpu.rlim_cur = strtoul(argv[2], NULL, 10);
	if (ret->limit_cpu.rlim_cur == 0
			|| ret->limit_cpu.rlim_cur == ULONG_MAX) {
		printf("Provide valid positive unsigned long integers.\n");
		free(ret->batch_name);
		free(ret);
		return NULL;
	}
	ret->limit_cpu.rlim_max = ret->limit_cpu.rlim_cur;

	ret->batch_name = strdup(argv[3]);

	return ret;
}

int free_command_bundle(command_bundle *command) {
	if (command == NULL) {
		return -1;
	}

	if (command->program_name != NULL) {
		free(command->program_name);
	}

	if (command->arguments != NULL) {
		unsigned int i;
		for (i = 0; i < command->argument_count; ++i) {
			if (command->arguments[i] != NULL) {
				free(command->arguments[i]);
			}
		}

		free(command->arguments);
	}

	free(command);

	return 0;
}

command_bundle * parse_command(char *command_buffer, int command_length) {
	command_bundle *command_ret = (command_bundle *) calloc(1, sizeof(command_bundle));
	if (command_ret == NULL) {
		free_command_bundle(command_ret);
		return NULL;
	}

	unsigned int argument_counter = 0;
	char *command_buffer_counter = strdup(command_buffer);
	char *token = strtok(command_buffer_counter, " \t");
	while (token != NULL) {
		++argument_counter;
		token = strtok(NULL, " \t");
	}
	if (argument_counter == 0) {
		command_ret->program_name = NULL;
		return command_ret;
	}

	command_ret->argument_count = argument_counter + 1;
	command_ret->arguments = (char **) calloc(command_ret->argument_count, sizeof(char *));
	if (command_ret->arguments == NULL) {
		free_command_bundle(command_ret);
		return NULL;
	}

	token = strtok(command_buffer, " \t");
	command_ret->program_name = strdup(token);
	command_ret->arguments[command_ret->argument_count - 1] = NULL;

	unsigned int i;
	for (i = 0; token != NULL; ++i) {
		command_ret->arguments[i] = strdup(token);
		token = strtok(NULL, " \t");
	}

	return command_ret;
}

int blame_command(const command_bundle *command, int exit_code) {
	printf("\nCommand \"");

	unsigned int i;
	for (i = 0; i < command->argument_count - 1; ++i) {
		printf("%s ", command->arguments[i]);
	}
	
	printf("\" failed with exit code %d.\n", exit_code);

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
				tv_ret.tv_usec = (size_t) (1E6 - (previous.tv_usec - present.tv_usec));
			}
		}
	}

	return tv_ret;
}

int report_time_stat(char *command_name, time_stat ts) {
	printf("\n================================="
			"\n| Command \"%s\" took"
			"\n|\t%ld.%06lds of user and"
			"\n|\t%ld.%06lds of system time.\n"
			"=================================\n\n",
			command_name,
			ts.user.tv_sec, ts.user.tv_usec,
			ts.sys.tv_sec, ts.sys.tv_usec);

	return 0;
}

time_stat * execute_command(const command_bundle *command, const limited_batch *batch_parameters) {
	time_stat *ts = (time_stat *) calloc(1, sizeof(time_stat));
	if (ts == NULL) {
		return NULL;
	}

	struct rusage present_usage, previous_usage;
	int child_pid = fork();
	if (child_pid == 0) {
		setrlimit(RLIMIT_AS, &(batch_parameters->limit_as));
		setrlimit(RLIMIT_CPU, &(batch_parameters->limit_cpu));
		execvp(command->program_name, command->arguments);
	} else if (child_pid > 0) {
		int status;
		getrusage(RUSAGE_CHILDREN, &previous_usage);
		error_code = waitpid(child_pid, &status, 0);
		if (error_code < 0) {
			free(ts);
			return NULL;
		}
		getrusage(RUSAGE_CHILDREN, &present_usage);

		if (WIFEXITED(status) != 0) {
			int exit_status = WEXITSTATUS(status);
			if (exit_status != 0) {
				blame_command(command, exit_status);
				free(ts);
				return NULL;
			} else {
				ts->user = time_difference(present_usage.ru_utime, previous_usage.ru_utime);
				ts->sys = time_difference(present_usage.ru_stime, previous_usage.ru_stime);
				return ts;
			}
		} else {
			printf("Some given usage limit has been broken.\n");
		}
	} else {
		printf("Process fork failed.\n");
		free(ts);
		return NULL;
	}

	return NULL;
}

int process_batch(const limited_batch *batch_parameters) {
	if (batch_parameters == NULL) {
		return -1;
	}

	FILE *batch = fopen(batch_parameters->batch_name, "rb");
	if (batch == NULL) {
		printf("Couldn't open %s.\n", batch_parameters->batch_name);
		return -2;
	}

	char *command_buffer;
	int command_length;
	int character;
	command_bundle *command;
	do {
		command_length = 0;
		do {
			character = fgetc(batch);
			++command_length;
		} while (character != '\n' && character != EOF);
		if (command_length == 1) {
			continue;
		}

		error_code = fseek(batch, -command_length, SEEK_CUR);

		command_buffer = (char *) calloc(command_length, sizeof(char));
		error_code = fread(command_buffer, sizeof(char), command_length, batch);
		if (error_code != sizeof(char) * command_length) {
			printf("Couldn't read line from batch.\n");
			return -3;
		}

		command_buffer[command_length - 1] = '\0';
		command = parse_command(command_buffer, command_length);

		if (command->program_name != NULL) {
			time_stat *ts = execute_command(command, batch_parameters);
			if (ts == NULL) {
				return -4;
			}
			report_time_stat(command->program_name, *ts);
			
			free(ts);
		}

		free_command_bundle(command);
		free(command_buffer);
	} while (character != EOF);

	fclose(batch);

	return 0;
}

int free_batch_parameters(limited_batch *batch_parameters) {
	if (batch_parameters == NULL) {
		return -1;
	}

	if (batch_parameters->batch_name != NULL) {
		free(batch_parameters->batch_name);
	}

	free(batch_parameters);

	return 0;

}

int main(int argc, char *argv[]) {
	limited_batch *batch_parameters = parse_args(argc, argv);
	if (batch_parameters == NULL) {
		printf("\n !! Parsing parameters failed. Exiting...\n");
		return 1;
	}

	error_code = process_batch(batch_parameters);
	if (error_code < 0) {
		printf("\n !! Couldn't execute the batch file. Exiting...\n");
		return 2;
	}

	free(batch_parameters);

	return 0;
}
