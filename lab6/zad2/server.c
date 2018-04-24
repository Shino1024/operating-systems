#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#include "info.h"

static int error_code;

static char finish_mode;

mqd_t client_ids[MAX_CLIENT_NUMBER];

static msg_buf received_message;

static unsigned int client_id_counter;

void clean_up() {
	printf("Cleaning up...\n");
	mq_close(server_descriptor);
	unsigned int i;
	for (i = 0; i < client_id_counter; ++i) {
		mq_close(client_ids[i]);
	}

	mq_unlink(server_name);
}

int main(int argc, char *argv[]) {
	if (atexit(clean_up) < 0) {
		perror("atexit");
		return 1;
	}
	if (signal(SIGINT, clean_up) == SIG_ERR) {
		perror("signal");
		return 1;
	}
	if (signal(SIGTERM, clean_up) == SIG_ERR) {
		perror("signal");
		return 1;
	}

	struct mq_attr attributes;
	memset(&attributes, 0, sizeof(attributes));
	attributes.mq_maxmsg = MAX_MESSAGES;
	attributes.mq_msgsize = MESSAGE_SIZE;
	server_descriptor = mq_open(server_name, O_CREAT | O_EXCL | O_RDONLY, 0666, &attributes);
	if (server_descriptor < 0) {
		perror("mq_open");
		return 1;
	}

	while (1) {
		if (finish_mode == 1) {
			struct mq_attr current_attributes;
			if (mq_getattr(server_descriptor, &current_attributes) < 0) {
				perror("mq_getattr");
				return 3;
			} else if (current_attributes.mq_curmsgs == 0) {
				printf("Finishing...\n");
				return 0;
			}
		}

		error_code = mq_receive(server_descriptor, (char *) (&received_message), sizeof(received_message), NULL);
		if (error_code < 0) {
			perror("mq_receive");
			return 3;
		} else if (error_code == 0) {
			printf("None read\n");
		} else {
			printf("READ: %d bytes \n\n\n", error_code);
			printf("[%d] :: %s\n", received_message.id, received_message.buffer);
			msg_buf return_msg;
			char *buffer_copy = strdup(received_message.buffer);
			strtok(buffer_copy, " \t");

			switch (received_message.mtype) {
				case KEY_ID:
					if (client_id_counter >= MAX_CLIENT_NUMBER) {
						printf("Unfortunately, this server cannot handle any new clients.\n");
						return_msg.mtype = END;
						break;
					}
					
					printf("Received %s\n", received_message.buffer);
					mqd_t received_descriptor = mq_open(received_message.buffer, O_WRONLY);
					if (received_descriptor < 0) {
						perror("mq_open");
						return 3;
					}

					return_msg.mtype = KEY_ID;
					return_msg.id = client_id_counter;
					client_ids[client_id_counter] = received_descriptor;

					break;

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

					return_msg.mtype = END;
					break;

				default:
					break;
			}

			free(buffer_copy);
			if (received_message.mtype == KEY_ID) {
				error_code = mq_send(client_ids[client_id_counter], (char *) (&return_msg), sizeof(return_msg), 1);
			} else {
				error_code = mq_send(client_ids[received_message.id], (char *) (&return_msg), sizeof(return_msg), 1);
			}
			if (error_code < 0) {
				perror("mq_send");
				return 3;
			}

			++client_id_counter;
		}
	}

	return 0;
}
