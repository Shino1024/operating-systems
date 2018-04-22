#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#include "info.h"

static int error_code;

static char finish_mode;

struct {
	int client_queue_id;
	key_t client_key;
} client_data[MAX_CLIENT_NUMBER];

static key_id_msg_buf received_key_message;
static msg_buf received_message;

static unsigned int client_id_counter;

void clean_up() {
	printf("Cleaning up...\n");
	msgctl(server_queue_id, IPC_RMID, NULL);
}

int main(int argc, char *argv[]) {
	atexit(clean_up);
	signal(SIGINT, clean_up);
	signal(SIGTERM, clean_up);

	home_env = getenv("HOME");
	if (home_env == NULL) {
		perror("getenv");
		return 1;
	}

	server_token = ftok(home_env, PROJECT_CHAR);
	if (server_token < 0) {
		perror("ftok");
		return 2;
	}

	server_queue_id = msgget(server_token, S_IRWXU | S_IRWXG | IPC_CREAT | IPC_EXCL);
	if (server_queue_id < 0) {
		perror("msgget");
		return 3;
	}

	while (1) {
		error_code = msgrcv(server_queue_id, &received_key_message, sizeof(received_key_message) - sizeof(received_key_message.mtype), KEY_ID, IPC_NOWAIT);
		if (error_code < 0) {
			if (errno != ENOMSG) {
				perror("msgrcv");
				return 3;
			}
		} else {
			if (client_id_counter >= MAX_CLIENT_NUMBER) {
				printf("Unfortunately, this server cannot handle any new clients.\n");
				continue;
			}
			int client_queue_id = msgget(received_key_message.key, 0);
			if (client_queue_id < 0) {
				perror("msgget");
				return 3;
			}

			key_id_msg_buf id;
			id.mtype = KEY_ID;
			id.key = client_id_counter;
			client_data[client_id_counter].client_key = received_key_message.key;
			client_data[client_id_counter].client_queue_id = client_queue_id;

			error_code = msgsnd(client_queue_id, &id, sizeof(int), IPC_NOWAIT);
			if (error_code < 0 && errno != EAGAIN) {
				perror("msgsnd");
				return 3;
			}

			++client_id_counter;
		}

		error_code = msgrcv(server_queue_id, &received_message, sizeof(received_message) - sizeof(received_message.mtype), KEY_ID, IPC_NOWAIT | MSG_EXCEPT);
		if (error_code < 0) {
			if (errno != ENOMSG) {
				perror("msgrcv");
				return 3;
			} else {
				if (finish_mode) {
					exit(0);
				}
			}
		} else {
			printf("[%d] :: %s\n", received_message.id, received_message.buffer);
			msg_buf return_msg;
			char *buffer_copy = strdup(received_message.buffer);
			strtok(buffer_copy, " \t");

			switch (received_message.mtype) {
				case MIRROR:
					{
						unsigned int iter;
						char *begin = strtok(NULL, "\n");
						size_t string_length = strlen(begin);
						char mirror[string_length + 1];
						for (iter = 0; iter < string_length; ++iter) {
							mirror[string_length - iter - 1] = begin[iter];
						}
						mirror[string_length] = '\0';

						return_msg.mtype = MIRROR;
						strcpy(return_msg.buffer, mirror);
					}
					break;

				case ADD:
					{
						char *first_number_string = strtok(NULL, " ");
						char *second_number_string = strtok(NULL, "\n");
						long first_number = strtol(first_number_string, NULL, 10);
						long second_number = strtol(second_number_string, NULL, 10);
						snprintf(return_msg.buffer, MAX_BUFFER_SIZE, "%ld", first_number + second_number);

						return_msg.mtype = ADD;
					}
					break;

				case SUB:
					{
						char *first_number_string = strtok(NULL, " ");
						char *second_number_string = strtok(NULL, "\n");
						long first_number = strtol(first_number_string, NULL, 10);
						long second_number = strtol(second_number_string, NULL, 10);
						snprintf(return_msg.buffer, MAX_BUFFER_SIZE, "%ld", first_number - second_number);

						return_msg.mtype = SUB;
					}
					break;

				case MUL:
					{
						char *first_number_string = strtok(NULL, " ");
						char *second_number_string = strtok(NULL, "\n");
						long first_number = strtol(first_number_string, NULL, 10);
						long second_number = strtol(second_number_string, NULL, 10);
						snprintf(return_msg.buffer, MAX_BUFFER_SIZE, "%ld", first_number * second_number);

						return_msg.mtype = MUL;
					}
					break;

				case DIV:
					{
						char *first_number_string = strtok(NULL, " ");
						char *second_number_string = strtok(NULL, "\n");
						long first_number = strtol(first_number_string, NULL, 10);
						long second_number = strtol(second_number_string, NULL, 10);
						snprintf(return_msg.buffer, MAX_BUFFER_SIZE, "%ld", second_number == 0 ? 0 : first_number / second_number);

						return_msg.mtype = DIV;
					}
					break;

				case TIME:
					{
						time_t now = time(NULL);
						snprintf(return_msg.buffer, MAX_BUFFER_SIZE, "%s", asctime(localtime(&now)));

						return_msg.mtype = TIME;
					}
					break;

				case END:
					finish_mode = 1;
					snprintf(return_msg.buffer, MAX_BUFFER_SIZE, "%s", "Finishing soon...");
					break;

				default:
					break;
			}

			free(buffer_copy);
			error_code = msgsnd(client_data[received_message.id].client_queue_id, &return_msg, sizeof(return_msg) - sizeof(return_msg.mtype), IPC_NOWAIT);
		}
	}

	return 0;
}
