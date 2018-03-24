#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

int error_code;

typedef struct command_bundle {
	char *program_name;
	char **arguments;
	int argument_count;
} command_bundle;

int usage(char *program_name) {
	printf("Usage: %s\n"
			"\n\tsource"
			"\n", program_name);

	return 0;
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
	printf("Command \"");

	unsigned int i;
	for (i = 0; i < command->argument_count - 1; ++i) {
		printf("%s ", command->arguments[i]);
	}
	
	printf("\" failed with exit code %d.\n", exit_code);

	return 0;
}

int execute_command(const command_bundle *command) {
	int child_pid = fork();
	if (child_pid == 0) {
		execvp(command->program_name, command->arguments);
	} else if (child_pid > 0) {
		int status;
		error_code = waitpid(child_pid, &status, 0);
		if (error_code < 0) {
			return -1;
		}

		if (WIFEXITED(status) != 0) {
			int exit_status = WEXITSTATUS(status);
			if (exit_status != 0) {
				blame_command(command, exit_status);
				return -3;
			} else {
				return 0;
			}
		}
		/*
		if (return_code != 0) {
			blame_command(command, return_code);
			return -1;
		} else {
			return 0;
		}
		*/
	} else {
		printf("Process fork failed.\n");
		return -2;
	}

	return 0;
}

int process_batch(const char *source) {
	FILE *batch = fopen(source, "rb");
	if (batch == NULL) {
		printf("Couldn't open %s.\n", source);
		return -1;
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
			return -2;
		}

		command_buffer[command_length - 1] = '\0';
		command = parse_command(command_buffer, command_length);

		if (command->program_name != NULL) {
			error_code = execute_command(command);
			if (error_code < 0) {
				return -3;
			}
		}

		free_command_bundle(command);
		free(command_buffer);
	} while (character != EOF);

	fclose(batch);

	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}

	error_code = process_batch(argv[1]);
	if (error_code < 0) {
		printf("Couldn't execute the batch file. Exiting...\n");
		return -1;
	}

	return 0;
}
