#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

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

static mqd_t private_descriptor;
static char private_name[MAX_QUEUE_NAME_LENGTH];

int id;

void generate_unique_queue_name(char *destination) {
	srand(time(NULL));
	destination[0] = '/';
	unsigned int i;
	for (i = 1; i < MAX_QUEUE_NAME_LENGTH - 1; ++i) {
		destination[i] = rand() % 26 + 64;
	}
	destination[i] = '\0';
}

void remove_queue() {
	printf("Cleaning up...\n");
	mq_close(private_descriptor);
	mq_close(server_descriptor);
	mq_unlink(private_name);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("The program should be called with one argument: the file containing the list of inquiries to be sent to server.\n");
		return 0;
	}

	//generate_unique_queue_name(private_name);
	sprintf(private_name, "/%d", getpid());

	if (atexit(remove_queue) < 0) {
		perror("atexit");
		return 1;
	}
	if (signal(SIGINT, remove_queue) == SIG_ERR) {
		perror("signal");
		return 1;
	}
	if (signal(SIGTERM, remove_queue) == SIG_ERR) {
		perror("signal");
		return 1;
	}

	struct mq_attr attributes;
	memset(&attributes, 0, sizeof(attributes));
	attributes.mq_maxmsg = MAX_MESSAGES;
	attributes.mq_msgsize = MESSAGE_SIZE;
	private_descriptor = mq_open(private_name, O_CREAT | O_EXCL | O_RDONLY, 0666, &attributes);
	if (private_descriptor < 0) {
		perror("mq_open");
		return 1;
	}

	server_descriptor = mq_open(server_name, O_WRONLY);
	if (server_descriptor < 0) {
		perror("mq_open");
		return -1;
	}
	
	msg_buf key_buf;
	key_buf.mtype = KEY_ID;
	key_buf.id = private_descriptor;
	strcpy(key_buf.buffer, private_name);
	printf("Sending %s|\n", key_buf.buffer);

	error_code = mq_send(server_descriptor, (char *) &key_buf, sizeof(key_buf), 1);
	if (error_code < 0) {
		perror("mq_send");
		return 2;
	}

	msg_buf id_buf;
	error_code = mq_receive(private_descriptor, (char *) &id_buf, sizeof(id_buf), NULL);
	if (error_code < 0) {
		perror("mq_receive");
		return 2;
	}
	id = id_buf.id;

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

			error_code = mq_send(server_descriptor, (char *) (&message), sizeof(message), 1);
			if (error_code < 0) {
				perror("mq_send");
				return 3;
			}

			msg_buf received_message;
			error_code = mq_receive(private_descriptor, (char *) (&received_message), sizeof(received_message), NULL);
			if (error_code < 0) {
				perror("mq_receive");
				return 3;
			}
			printf("Received from server: %s\n\n", received_message.buffer);
		}
	}

	fclose(input);

	return 0;
}
