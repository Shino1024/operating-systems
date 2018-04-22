#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "info.h"

static int error_code;

static char *operations[] = {
	"MIRROR",
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"TIME",
	"END"
};

int private_queue_id;

int id;

void remove_queue() {
	printf("Cleaning up...\n");
	msgctl(private_queue_id, IPC_RMID, NULL);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("The program should be called with one argument: the file containing the list of inquiries to be sent to server.\n");
		return 0;
	}

	atexit(remove_queue);
	
	home_env = getenv("HOME");
	if (home_env == NULL) {
		perror("getenv");
		return 1;
	}

	server_token = ftok(home_env, PROJECT_CHAR);
	if (server_token == -1) {
		perror("ftok");
		return 2;
	}
	server_queue_id = msgget(server_token, S_IRWXU | S_IRWXG);
	if (server_queue_id < 0) {
		perror("msgget");
		return 2;
	}

	key_t private_token = ftok(home_env, getpid());
	if (private_token == -1) {
		perror("ftok");
		return 2;
	}
	private_queue_id = msgget(private_token, S_IRWXU | S_IRWXG | IPC_CREAT | IPC_EXCL);
	if (private_queue_id < 0) {
		perror("msgget");
		return 2;
	}

	key_id_msg_buf key_buf;
	key_buf.mtype = KEY_ID;
	key_buf.key = private_token;

	error_code = msgsnd(server_queue_id, &key_buf, sizeof(key_buf) - sizeof(key_buf.mtype), 0);
	if (error_code < 0) {
		perror("msgsnd");
		return 2;
	}

	key_id_msg_buf id_buf;
	error_code = msgrcv(private_queue_id, &id_buf, sizeof(id_buf) - sizeof(id_buf.mtype), KEY_ID, 0);
	if (error_code < 0) {
		perror("msgrcv");
		return 2;
	}
	id = id_buf.key;

	FILE *input = fopen(argv[1], "r");
	if (input == NULL) {
		perror("fopen");
		return 3;
	}

	unsigned int i;
	char command_buffer[MAX_COMMAND_LINE_SIZE];
	while (fgets(command_buffer, MAX_COMMAND_LINE_SIZE, input)) {
		char *command_buffer_copy = strdup(command_buffer);
		char *command_name = strtok(command_buffer_copy, " \t\n");
		if (command_name == NULL) {
			free(command_buffer_copy);
			continue;
		}
		char command_matched = 0;
		for (i = 0; i < sizeof(operations) / sizeof(*operations); ++i) {
			if (strcmp(command_name, operations[i]) == 0) {
				command_matched = 1;
				break;
			}
		}

		if (command_matched == 0) {
			fprintf(stderr, "Unrecognized command %s found.\n", command_name);
			free(command_buffer_copy);
			continue;
		} else {
			msg_buf message;
			message.mtype = MIRROR + i;
			message.id = id;
			strcpy(message.buffer, command_buffer);
			free(command_buffer_copy);

			error_code = msgsnd(server_queue_id, &message, sizeof(message) - sizeof(message.mtype), IPC_NOWAIT);
			if (error_code < 0) {
				perror("msgsnd");
				return 3;
			}

			msg_buf received_message;
			error_code = msgrcv(private_queue_id, &received_message, sizeof(received_message) - sizeof(received_message.mtype), KEY_ID, MSG_EXCEPT);
			if (error_code < 0) {
				perror("msgrcv");
				return 3;
			}
			printf("Received from server: %s\n\n", received_message.buffer);
		}
	}

	fclose(input);

	return 0;
}
