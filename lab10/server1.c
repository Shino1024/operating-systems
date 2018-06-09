#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include "info.h"

#ifndef UNIX_MAX_PATH
#	define UNIX_MAX_PATH 108
#endif

#define MAX_EPOLL_EVENTS 10

#define MAX_BUFFER_SIZE 256

#define MAX_CLIENTS_NUMBER 20

#define PING_SECONDS 2

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
	char type;
	union {
		struct sockaddr_un unix;
		struct sockaddr_in inet;
	} addr;
} clients_data[MAX_CLIENTS_NUMBER];

struct {
	char *buffer;
	char new_input_available;
	unsigned int operation_counter;
} server_data;

struct {
	int register_epoll_fd;
	int operation_epoll_fd;
} epolls;

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
/*
void * broadcast_routine(void *arg) {
	for (;;) {
		pthread_mutex_lock(&buffer_mutex);

		//

		pthread_mutex_unlock(&buffer_mutex);
	}

	pthread_exit(NULL);
}
*/
int configure_epolls() {
	socklen_t data_len;
	int client_fd;
	struct epoll_event event_unix, event_inet;

	event_unix.events = EPOLLIN;
	event_unix.data.fd = server_info.unix_sock;

	event_inet.events = EPOLLIN;
	event_inet.data.fd = server_info.inet_sock;

	epolls.register_epoll_fd = epoll_create1(0);
	if (epolls.register_epoll_fd < 0) {
		fprintf(stderr, "Couldn't create a register epoll.\n");
		return -1;
	}	

	if (epoll_ctl(epolls.register_epoll_fd, EPOLL_CTL_ADD, 0, &event_unix) != 0) {
		fprintf(stderr, "Couldn't add unix event to epoll.\n");
		return -1;
	}

	if (epoll_ctl(epolls.register_epoll_fd, EPOLL_CTL_ADD, 0, &event_inet) != 0) {
		fprintf(stderr, "Couldn't add inet event to epoll.\n");
		return -1;
	}

	return 0;
}

{

		//pthread_mutex_lock(&clients_mutex);

//		data_len = sizeof(*addr);
//		client_fd = accept(sock, addr, &data_len);
//		if (client_fd > 0 || data_len == sizeof(*addr)) {
//			server_info.
//		} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
//			fprintf(stderr, "Accepting new client failed.\n");
//			pthread_exit(NULL);
//		}

		//

		//pthread_mutex_unlock(&clients_mutex);
}

unsigned int find_available_index() {
	unsigned int iter;
	for (iter = 0; iter < MAX_CLIENT_NUMBER; ++iter) {
		if (clients_data[iter].sock == 0) {
			return iter;
		}
	}

	return UINT_MAX;
}

int name_already_registered(char *name) {
	for () {
	}

	return 1;
}

int reject_client(int client_sock, message_type reason) {
	message m;
	m.message_type = reason;
	unsigned int written_bytes = write(client_sock, &m.message_type, sizeof(m.message_type));
	close(client_sock);

	return 0;
}

int save_client(int client_sock, unsigned int new_index, char type, void *addr_union) {
	clients_data[new_index].sock = client_sock;
	clients_data[new_index].type = type;
	memcpy(&(clients_data[new_index].addr), addr_union, sizeof(*addr_union));

	message m;
	m.message_type = REGISTERED;
	unsigned int written_bytes = write(client_sock, &m.message_type, sizeof(m.message_type));
	
	return 0;
}

int handle_client(struct epoll_event event) {
	if (server_info.unix_sock == event.data.fd || server_info.inet_sock == event.data.fd) {
		int client_sock;
		union {
			struct sockaddr_un unix;
			struct sockaddr_in inet;
		} addr;
		if (server_info.unix_sock == event.data.fd) {
			client_sock = accept(event.data.fd, (const struct sockaddr *) (&addr.unix), sizeof(addr.unix));
			if (client_sock == -1) {
				fprintf(stderr, "Couldn't accept unix client\n");
				return -1;
			}
		}
		if (server_info.inet_sock == event.data.fd) {
			client_sock = accept(event.data.fd, (const struct sockaddr *) (&addr.inet), sizeof(addr.inet));
			if (client_sock == -1) {
				fprintf(stderr, "Couldn't accept inet client\n");
				return -1;
			}
		}

		unsigned int new_index = find_available_index();
		if (new_index == UINT_MAX) {
			reject_client(client_sock, ALREADY_FULL);
		} else {
			message m;
			unsigned int header_length = read(event.data.fd, &m, sizeof(m) - sizeof(m.data));
			m.data = calloc(sizeof(1), m.message_length + 3);
			if (m.data == NULL) {
				fprintf(stderr, "NULL in calloc\n");
				return -2;
			}
			
			memset(m.data, 0, m.message_length + 3);
			unsigned int data_length = read(event.data.fd, m.data, m.message_length);
			if (name_already_registered(m.data) > 0) {
				reject_client(client_sock, NAME_BUSY);
			} else {
				save_client(client_sock, new_index);
			}
		}
	}

	return 0;
}

int register_clients() {
	struct epoll_event received_events[MAX_EPOLL_EVENTS];
	unsigned int event_count;
	unsigned int event_iter;
	unsigned int bytes_read;
	message message_in;

	for (;;) {
		memset(received_events, 0, sizeof(received_events));
		event_count = epoll_wait(epolls.register_epoll_fd, received_events, MAX_EPOLL_EVENTS, 0);
		if (event_count < 0) {
			fprintf(stderr, "An error occured in epoll_wait.\n");
			return -1;
		}

		for (event_iter = 0; event_iter < event_count; ++event_iter) {
			handle_client(received_events[event_iter]);
			bytes_read = read(received_events[event_iter].data.fd, &message_in, sizeof(message_in) - sizeof(message_in.data));
			if (bytes_read <= 0) {
				fprintf(stderr, "Couldn't read a message from incoming client.\n");
				return -2;
			}


		}
	}

	return 0;
}

int unregister_client() {
	return 0;
}

void * ping_routine(void *arg) {
	unsigned int i;
	for (;;) {
		sleep(PING_SECONDS);

		for (i = 0; i < ) {
		}
	}

	pthread_exit(NULL);
}

int configure_input() {
	int ret = fcntl(0, F_GETFL);
	if (ret < 0) {
		fprintf(stderr, "Couldn't manipulate on stdin.\n");
		return -1;
	}

	if (fcntl(0, F_SETFL, ret | O_NONBLOCK) == -1) {
		fprintf(stderr, "Couldn't manipulate on stdin.\n");
		return -1;
	}

	return 0;
}

int init_server() {
	server_info.unix_sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0); // TODO: nonblock?
	if (server_info.unix_sock < 0) {
		fprintf(stderr, "Couldn't init unix_sock.\n");
		return -1;
	}

	server_info.inet_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (server_info.inet_sock < 0) {
		fprintf(stderr, "Couldn't init inet_sock.'n");
		return -1;
	}

	server_info.addr_unix.sun_family = AF_UNIX;
	strcpy(server_info.addr_unix.sun_path, args.unix_socket);

	server_info.addr_inet.sin_family = AF_INET;
	server_info.addr_inet.sin_port = htons(args.port); // TODO: htons? htonl?
	server_info.addr_inet.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(server_info.unix_sock, (const struct sockaddr *) (&server_info.addr_unix), SUN_LEN(&server_info.addr_unix)) < 0) { // TODO: SUN_LEN?
		fprintf(stderr, "Couldn't bind unix sock to address.\n");
		return -2;
	}

	if (bind(server_info.inet_sock, (const struct sockaddr *) (&server_info.addr_inet), sizeof(server_info.addr_inet)) < 0) {
		fprintf(stderr, "Couldn't bind inet sock to address.\n");
		return -2;
	}

	if (listen(server_info.unix_sock, MAX_CLIENTS_NUMBER) < 0) {
		fprintf(stderr, "Couldn't listen on unix sock.\n");
		return -3;
	}

	if (listen(server_info.inet_sock, MAX_CLIENTS_NUMBER) < 0) {
		fprintf(stderr, "COuldn't listen on inet_sock.\n");
		return -3;
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
/*
int dispatch_threads() {
	pthread_t *thread_ids[] = {
		&threads.ping_routine_id,
		&threads.broadcast_routine_id,
		&threads.register_routine_id
	};

	void * (* routines[])(void *) = {
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
*/
int handle_input() {
	//
	return 0;
}

void main_loop() {
	for (;;) {
		//pthread_mutex_lock(&buffer_mutex);

		register_clients();
		handle_input();
		broadcast_operation();
		clear_buffer();
		receive_result();
		print_result();

/*
		read_input();
		set_new_input();
*/
		//pthread_mutex_unlock(&buffer_mutex);
	}
}

int print_usage(char *program) {
	printf("Usage: %s"
			"\n\tTCP port number (between 1024 and 65535 inclusive"
			"\n\tUNIX socket pathname"
			"\n", program);

	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		print_usage(argv[0]);
		return 0;
	}

	if (parse_args(argc, argv) < 0) {
		return 1;
	}

	if (configure_input() < 0) {
		return 2;
	}

	if (configure_epolls() < 0) {
		return 3;
	}

	if (init_server() < 0) {
		return 3;
	}

	/*
	if (dispatch_threads() < 0) {
		fprintf(stderr, "Couldn't dispatch threads. Exiting...\n");
		return 3;
	}
	*/

	main_loop();

	return 0;
}

