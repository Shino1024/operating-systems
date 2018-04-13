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

int execute_connected_commands(const command_bundle *command_line) {
	unsigned int i;
	unsigned int pipe_count = 0;
	unsigned int *first_words;
	unsigned int *last_words;
	int (*pipes)[2];
	if (strcmp(command_line->arguments[0], "|") == 0) {
		printf("Found a pipe at the beginning of line.\n");
		return -1;
	}

	if (strcmp(command_line->arguments[command_line->argument_count - 1], "|") == 0) {
		printf("Found a pipe at the end of line.\n");
		return -1;
	}
	for (i = 0; i < command_line->argument_count - 1; ++i) {
		if (strcmp(command_line->arguments[i], "|") == 0) {
			if (strcmp(command_line->arguments[i + 1], "|") == 0) {
				printf("Found a second pipe at word %d.\n", i + 1);
				return -1;
			} else {
				++pipe_count;
			}
		}
	}

	first_words = (unsigned int *) calloc(pipe_count + 1, sizeof(unsigned int));
	if (first_words == NULL) {
		return -2;
	}
	last_words = (unsigned int *) calloc(pipe_count + 1, sizeof(unsigned int));
	if (last_words == NULL) {
		free(first_words);
		return -2;
	}

	pipes = (int(*)[2]) calloc(pipe_count, sizeof(int [2]));
	if (pipes == NULL) {
		free(first_words);
		free(last_words);
		return -2;
	}

	unsigned int words_counter = 0;
	first_words[0] = 0;
	last_words[pipe_count] = command_line->argument_count - 1;
	for (i = 0; i < command_line->argument_count - 1; ++i) {
		if (strcmp(command_line->arguments[i], "|") == 0) {
			last_words[words_counter] = i - 1;
			++words_counter;
			first_words[words_counter] = i + 1;
		}
	}

	pid_t child_pid;
	unsigned int command;
	for (command = 0; command < pipe_count; ++command) {
		error_code = pipe(pipes[command]);
		if (error_code < 0) {
			perror("Couldn't set up a pipe.\n");
			kill(0, SIGKILL);
			return -3;
		}
	}

	for (command = 0; command <= pipe_count; ++command) {	
		child_pid = fork();
		if (child_pid < 0) {
			perror("Couldn't create a child, killing all children...\n");
			kill(0, SIGKILL);
			return -4;
		} else if (child_pid == 0) {
			unsigned int argument_count = last_words[command] - first_words[command] + 1;
			char **arguments = (char **) calloc(argument_count + 1, sizeof(char *));
			unsigned int argument;
			for (argument = 0; argument < argument_count; ++argument) {
				arguments[argument] = strdup(command_line->arguments[first_words[command] + argument]);
				printf("DBG: %d / %d / %s\n", command, argument, arguments[argument]);
			}
			arguments[argument_count] = NULL;

			if (command == 0) {
				close(pipes[0][0]);
				dup2(pipes[0][1], STDOUT_FILENO);
			} else if (command == pipe_count) {
				close(pipes[pipe_count - 1][1]);
				dup2(pipes[pipe_count - 1][0], STDIN_FILENO);
			} else {
				dup2(pipes[command][1], STDOUT_FILENO);
				dup2(pipes[command][0], STDIN_FILENO);
			}

			execvp(arguments[0], arguments);
			exit(1);
		}

		int status;
		while (wait(&status) > 0);
	}
	/*
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
	} else {
		printf("Process fork failed.\n");
		return -2;
	}
	*/

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
			error_code = execute_connected_commands(command);
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
