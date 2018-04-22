#ifndef INfO__H
#define INFO__H

#define SERVER_IPC_TOKEN 0xDEADBEEF
#define PROJECT_CHAR 's'

#define MAX_CLIENT_NUMBER 1024

#define MAX_BUFFER_SIZE 1024

#define MAX_COMMAND_LINE_SIZE 1024

typedef enum msg_type {
	KEY_ID = 1,
	MIRROR,
	ADD,
	SUB,
	MUL,
	DIV,
	TIME,
	END
} msg_type;

typedef struct key_id_msg_buf {
	long mtype;
	int key;
} key_id_msg_buf;

typedef struct msg_buf {
	long mtype;
	int id;
	char buffer[MAX_BUFFER_SIZE];
} msg_buf;

key_t server_token;
int server_queue_id;
char *home_env;

#endif // INFO__H
