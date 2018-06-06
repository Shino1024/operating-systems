#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "info.h"

#ifndef UNIX_MAX_PATH
#	define UNIX_MAX_PATH 108
#endif

#define MAX_CLIENTS_NUMBER 20

struct {
	unsigned int port;
	char *unix_socket;
} args;

struct {
	char **clients;
	int inet_sock;
	int unix_sock;
	struct sockaddr_un addr_unix;
	struct sockaddr_in addr_inet;
} server_info;

struct {
	pthread_t ping_routine_id;
	pthread_t broadcast_routine_id;
	pthread_t register_routine_id;
} threads;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

struct {
	char *name;
	int sock;
	struct sockaddr addr;
} clients_data[MAX_CLIENTS_NUMBER];

struct {
	char *buffer;
} server_data;

int parse_port(char *port) {
	char *fail;
	args.port = (unsigned int) strtoul(port, &fail, 10);
	if (*fail != "\0" || args.port < 1024 || args.port >= (1 << 16)) {
		return -1;
	}

	return 0;
}

int parse_unix_socket(char *unix_socket) {
	args.unix_socket = strdup(unix_socket);
	if (args.unix_socket == NULL) {
		return -1;
	}

	if (strlen(args.unix_socket) > UNIX_PATH_MAX) {
		fprintf(stderr, "Make sure that the server's address doesn't exceed %d characters in length.\n", UNIX_MAX_PATH);
		return -2;
	}

	if (fopen(args.unix_socket, "r") == NULL) {
		return -3;
	} else {
		fclose(args.unix_socket);
		return 0;
	}
}

int parse_args(int argc, char *argv[]) {
	if (parse_port(argv[1]) < 0) {
		fprintf(stderr, "Port number must be between 1024 and 65535 inclusive.\n");
		return -1;
	}

	if (parse_unix_socket(argv[2]) < 0) {
		fprintf(stderr, "The provided UNIX socket path isn't correct.\n");
		return -2;
	}

	return 0;
}

void * broadcast_routine(void *arg) {
	for (;;) {
		pthread_mutex_lock(&buffer_mutex);

		//

		pthread_mutex_unlock(&buffer_mutex);
	}

	pthread_exit(NULL);
}

void * register_routine(void *arg) {
	socklen_t data_len;
	int client_fd;
	for (;;) {
		pthread_mutex_lock(&clients_mutex);

//		data_len = sizeof(*addr);
//		client_fd = accept(sock, addr, &data_len);
//		if (client_fd > 0 || data_len == sizeof(*addr)) {
//			server_info.
//		} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
//			fprintf(stderr, "Accepting new client failed.\n");
//			pthread_exit(NULL);
//		}

		//

		pthread_mutex_unlock(&clients_mutex);
	}

	pthread_exit(NULL);
}

void * ping_routine(void *arg) {
	for (;;) {
		sleep(1);

		//
	}
	pthread_exit(NULL);
}

int init_server() {
	server_info.unix_sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (server_info.unix_sock < 0) {
		fprintf(stderr, "Couldn't init unix_sock.\n");
		return -1;
	}

	server_info.inet_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (server_info.inet_sock < 0) {
		fprintf(stderr, "Couldn't init inet_sock.'n");
		return -2;
	}

	server_info.addr_unix.sun_family = AF_UNIX;
	strcpy(server_info.addr_unix.sun_path, args.unix_socket);

	server_info.addr_inet.sin_family = AF_INET;
	server_info.addr_inet.sin_port = args.port;
	server_info.addr_inet.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_info.unix_sock, (const struct sockaddr *) (&server_info.addr_unix), sizeof(server_info.addr_unix)) < 0) {
		fprintf(stderr, "Couldn't bind sock to address.\n");
		return -3;
	}

	if (listen(server_info.unix_sock, MAX_CLIENTS_NUMBER) < 0) {
		fprintf(stderr, "Couldn't listen on unix_sock.\n");
		return -4;
	}

	return 0;
}

int create_detached_thread(pthread_t *thread_id, void (* routine)(void *)) {
	pthread_attr_t thread_attr;
	if (pthread_attr_init(&thread_attr) != 0) {
		fprintf(stderr, "Couldn't init thread attributes.\n");
		return -1;
	}

	if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DEATCHED) != 0) {
		fprintf("Couldn't assign the detached attribute to thread attribute set.\n");
		return -1;
	}

	if (pthread_create(thread_id, &thread_attr, routine, NULL) != 0) {
		fprintf("Couldn't create thread.\n");
		return -1;
	}

	return 0;
}

int dispatch_threads() {
	pthread_t *thread_ids[] = {
		&threads.ping_routine_id,
		&threads.broadcast_routine_id,
		&threads.register_routine_id
	};

	void (* routines[])(void *) = {
		&ping_routine,
		&broadcast_routine,
		&register_routine
	};

	unsigned int iter;
	for (iter = 0; iter < sizeof(thread_ids) / sizeof(*thread_ids); ++iter) {
		if (create_detached_thread(thread_ids[iter], routines[iter]) < 0) {
			return -1;
		}
	}

	return 0;
}

void main_loop() {
	for (;;) {
	}
}

int print_usage(char *program) {
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		print_usage(argv[0]);
		return 0;
	}

	if (parse_args(argc, argv) < 0) {
		fprintf(stderr, "Couldn't parse args. Exiting...\n");
		return 1;
	}

	if (init_server() < 0) {
		return 2;
	}

	if (dispatch_threads() < 0) {
		fprintf(stderr, "Couldn't dispatch threads. Exiting...\n");
		return 3;
	}

	main_loop();

	return 0;
}

