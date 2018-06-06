typedef enum message_type {
	REGISTER,
	NAME_BUSY,
	REGISTERED,
	PING,
	PONG,
	COMPUTE_REQUEST,
	COMPUTE_RESULT,
	UNREGISTER,
	ERROR
} message_type;

typedef enum operation {
	ADD,
	SUB,
	MUL,
	DIV,
	MOD
} operation;

typedef struct expression {
	long left;
	long right;
	operation op;
} expression;

typedef struct message {
	message_type message_type;
	unsigned int message_length;
	expression expression;
} message;

