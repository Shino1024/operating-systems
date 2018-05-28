#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>

#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET "\x1b[0m"

#define MAX_P 32
#define MAX_K 128
#define MAX_N 1048576
#define MAX_L 8192

#define STR(M) #M

struct {
	pthread_t *producers;
	pthread_t *consumers;
	pthread_mutex_t single_elem;
	sem_t num_in;
	sem_t num_out;
} threads;

typedef enum search_mode {
	LT,
	EQ,
	GT
} search_mode;

typedef enum log_mode {
	SIMPLE,
	BOTH
} log_mode;

struct {
	char *config_filename;
	unsigned int p;
	unsigned int k;
	unsigned int n;
	char *filename;
	unsigned int l;
	search_mode sm;
	log_mode lm;
	unsigned int nk;
} args;

struct {
	FILE *f;
	char **buf;
	unsigned int taken;
	unsigned int last_pos_in;
	unsigned int last_pos_out;
} buf;

unsigned int parse_uint(char *uint, unsigned int limit) {
	char *bad_char;
	unsigned int ret = (int) strtoul(uint, &bad_char, 10);
	if (*bad_char != '\0') {
		return UINT_MAX;
	}

	if (limit > 0) {
		if (ret >= limit) {
			return UINT_MAX;
		}
	}

	return ret;
}

void int_handler(int s) {
	if (buf.f != NULL) {
		pthread_mutex_lock(&(threads.single_elem));
		fclose(buf.f);
		buf.f = NULL;
	}
	printf(ANSI_RESET);
	exit(0);
}

void err_quit(const char *msg, int code) {
	fprintf(stderr, "\n%s Exiting...\n", msg);
	exit(code);
}

void print_usage(char *program_name) {
	printf("Usage: %s"
			"\n\tconfig_file, WHICH CONTAINS:"
			"\n\t\tnum_of_producers"
			"\n\t\tnum_of_consumers"
			"\n\t\tbuffer_size"
			"\n\t\tfilename_to_process"
			"\n\t\tline_length"
			"\n\t\tsearch_mode (LT|EQ|GT)"
			"\n\t\tlog_mode (simple|both)"
			"\n\t\tmax_seconds_per_thread"
			"\n", program_name);
}

void parse_args(char *config) {
	char *seps = " \r\n\t";
	char *b;

	if ((b = strtok(config, seps)) == NULL) {
		err_quit("Reached end of config file before reading all arguments.", 4);
	}
	args.p = parse_uint(b, MAX_P);
	if (args.p == UINT_MAX) {
		err_quit("num_of_producers should be a positive number, max" STR(MAX_P) ".", 2);
	}

	if ((b = strtok(NULL, seps)) == NULL) {
		err_quit("reached end of config file before reading all arguments.", 4);
	}
	args.k = parse_uint(b, MAX_K);
	if (args.k == UINT_MAX) {
		err_quit("num_of_consumers should be a positive number, max" STR(MAX_K) ".", 2);
	}

	if ((b = strtok(NULL, seps)) == NULL) {
		err_quit("reached end of config file before reading all arguments.", 4);
	}
	args.n = parse_uint(b, MAX_N);
	if (args.n == UINT_MAX) {
		err_quit("buffer_size should be a positive number, max" STR(KAX_N) ".", 2);
	}

	if ((b = strtok(NULL, seps)) == NULL) {
		err_quit("reached end of config file before reading all arguments.", 4);
	}
	args.filename = strdup(b);
	if (args.filename == NULL) {
		err_quit("Couldn't allocate helper memory.", 2);
	}

	if ((b = strtok(NULL, seps)) == NULL) {
		err_quit("reached end of config file before reading all arguments.", 4);
	}
	args.l = parse_uint(b, MAX_L);
	if (args.l == UINT_MAX) {
		err_quit("Line length should be a positive number, max" STR(MAX_L) ".", 2);
	}

	if ((b = strtok(NULL, seps)) == NULL) {
		err_quit("reached end of config file before reading all arguments.", 4);
	}
	if (strcmp(b, "LT") == 0) {
		args.sm = LT;
	} else if (strcmp(b, "EQ") == 0) {
		args.sm = EQ;
	} else if (strcmp(b, "GT") == 0) {
		args.sm = GT;
	} else {
		err_quit("search_mode should be one of LT, EQ or GT.", 2);
	}

	if ((b = strtok(NULL, seps)) == NULL) {
		err_quit("reached end of config file before reading all arguments.", 4);
	}
	if (strcmp(b, "simple") == 0) {
		args.lm = SIMPLE;
	} else if (strcmp(b, "both") == 0) {
		args.lm = BOTH;
	} else {
		err_quit("log_mode should be one of simple or both.", 2);
	}

	if ((b = strtok(NULL, seps)) == NULL) {
		err_quit("reached end of config file before reading all arguments.", 4);
	}
	args.nk = parse_uint(b, 0);
	if (args.nk == UINT_MAX) {
		err_quit("max_seconds_per_thread should be a non-negative number.", 2);
	}
}

char * read_config() {
	FILE *f;
	char *config;

	f = fopen(args.config_filename, "r");
	if (f == NULL) {
		err_quit("Couldn't open config file.", 4);
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);

	fseek(f, 0, SEEK_SET);
	config = calloc(sizeof(char), size + 1);
	if (config == NULL) {
		err_quit("Couldn't allocate memory for config.", 3);
	}

	if (fread(config, size, sizeof(char), f) < 1) {
		err_quit("Couldn't read contents from file.", 4);
	}

	fclose(f);

	return config;
}

char * get_line() {
	char *line = (char *) calloc(sizeof(char), MAX_L + 2);
	if (line == NULL) {
		err_quit("Couldn't allocate helper memory.", 3);
	}

	if (fgets(line, MAX_L, buf.f) == NULL) {
		pthread_exit(NULL);
	}

	return line;
}

void insert_line(unsigned int thread_no, char *line) {
	if (args.lm == BOTH) {
		printf(ANSI_GREEN "[INS %d AT %d] :: %s" ANSI_RESET, thread_no, buf.last_pos_in, line);
	}
	buf.buf[buf.last_pos_in] = strdup(line);
	if (buf.buf[buf.last_pos_in] == NULL) {
		err_quit("Couldn't allocate buffer memory.", 3);
	}
	buf.last_pos_in = (buf.last_pos_in + 1) % args.n;

	free(line);
}

void print_line(unsigned int thread_no) {
	char condition_met = 0;
	switch (args.sm) {
		case LT:
			if (strlen(buf.buf[buf.last_pos_out]) < args.l) {
				condition_met = 1;
			}
			break;

		case EQ:
			if (strlen(buf.buf[buf.last_pos_out]) == args.l) {
				condition_met = 1;
			}
			break;

		case GT:
			if (strlen(buf.buf[buf.last_pos_out]) > args.l) {
				condition_met = 1;
			}
			break;

		default:
			break;
	}

	if (condition_met) {
		printf(ANSI_YELLOW "[GET %d AT %d] :: %s" ANSI_RESET, thread_no, buf.last_pos_out, buf.buf[buf.last_pos_out]);
	}

	free(buf.buf[buf.last_pos_out]);
	buf.buf[buf.last_pos_out] = NULL;
	buf.last_pos_out = (buf.last_pos_out + 1) % args.n;
}

void * producer_thread(void *arg) {
	int thread_no = *((int *) arg);
	free(arg);
	for (;;) {
		char *line = get_line();
		sem_wait(&(threads.num_out));
		pthread_mutex_lock(&(threads.single_elem));
		insert_line(thread_no, line);

		++(buf.taken);
		pthread_mutex_unlock(&(threads.single_elem));
		sem_post(&(threads.num_in));
	}

	pthread_exit(NULL);
}

void * consumer_thread(void *arg) {
	int thread_no = *((int *) arg);
	free(arg);
	for (;;) {
		sem_wait(&(threads.num_in));
		pthread_mutex_lock(&(threads.single_elem));

		print_line(thread_no);

		--(buf.taken);
		sem_post(&(threads.num_out));
		pthread_mutex_unlock(&(threads.single_elem));
	}

	pthread_exit(NULL);
}

void dispatch_producers() {
	unsigned int i;
	for (i = 0; i < args.p; ++i) {
		int *arg = (int *) calloc(sizeof(int), 1);
		if (arg == NULL) {
			err_quit("Couldn't allocate helper memory.", 3);
		}
		*arg = i;
		pthread_create(threads.producers + i, NULL, producer_thread, arg);
	}
}

void dispatch_consumers() {
	unsigned int i;
	for (i = 0; i < args.k; ++i) {
		int *arg = (int *) calloc(sizeof(int), 1);
		if (arg == NULL) {
			err_quit("Couldn't allocate helper memory.", 3);
		}
		*arg = i;
		pthread_create(threads.consumers + i, NULL, consumer_thread, arg);
	}
}

void sync_threads() {
	unsigned int i;
	for (i = 0; i < args.p; ++i) {
		pthread_join(threads.producers[i], (void **) NULL);
	}

	for (i = 0; i < args.k; ++i) {
		pthread_cancel(threads.consumers[i]);
	}
}

void dispatch_threads() {
	buf.f = fopen(args.filename, "r");
	if (buf.f == NULL) {
		err_quit("Couldn't open the file from config.", 4);
	}

	dispatch_producers();
	dispatch_consumers();
	if (args.nk > 0) {
		alarm((int) args.nk);
	}

	sync_threads();

	fclose(buf.f);
}

void atexit_handler() {
	printf(ANSI_RESET);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		print_usage(argv[0]);
		exit(0);
	}
	args.config_filename = argv[1];

	if (pthread_mutex_init(&(threads.single_elem), NULL) != 0) {
		err_quit("Couldn't init a mutex.", 1);
	}

	if (sem_init(&(threads.num_in), 0, 0) < 0) {
		err_quit("Couldn't init an elements-in-buffer semaphore.", 1);
	}

	char *config = read_config();
	parse_args(config);

	if (sem_init(&(threads.num_out), 0, args.n) < 0) {
		err_quit("Couldn't init an elements-NOT-in-buffer semaphore.", 1);
	}

	if (atexit(atexit_handler) != 0) {
		err_quit("Couldn't set up an exit handler.", 1);
	}

	threads.producers = (pthread_t *) calloc(sizeof(pthread_t), args.p);
	if (threads.producers == NULL) {
		err_quit("Couldn't allocate memory for producer thread IDs.", 3);
	}

	threads.consumers = (pthread_t *) calloc(sizeof(pthread_t), args.k);
	if (threads.consumers == NULL) {
		err_quit("Couldn't allocate memory for consumer thread IDs.", 3);
	}

	buf.buf = (char **) calloc(sizeof(char *), args.n);
	if (buf.buf == NULL) {
		err_quit("Couldn't allocate memory for buffer.", 3);
	}
	unsigned int i;
	for (i = 0; i < args.n; ++i) {
		buf.buf[i] = NULL;
	}

	dispatch_threads();

	return 0;
}

