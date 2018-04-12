#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_CHILDREN 32

typedef enum settings {
	CHILDREN_PID = 0,
	INQUIRIES,
	ACCEPTANCES,
	REALTIME_SIGNALS,
	CHILD_FINISHED
} settings;

#define IS_CHILDREN_PID_SET(settings) ((settings) & (1 << CHILDREN_PID))
#define IS_INQUIRIES_SET(settings) ((settings) & (1 << INQUIRIES))
#define IS_ACCEPTANCES_SET(settings) ((settings) & (1 << ACCEPTANCES))
#define IS_REALTIME_SIGNALS_SET(settings) ((settings) & (1 << REALTIME_SIGNALS))
#define IS_CHILD_FINISHED_SET(settings) ((settings) & (1 << CHILD_FINISHED))

#define MAX_SLEEP_SECONDS 10

static int error_code;

static unsigned long children, inquiries;
static unsigned long received_inquiries;
static pid_t *inquiry_pids;
static unsigned long children_exited;
static unsigned long realtime_signals_received;

static char settings_storage;

static unsigned int sleep_seconds;

static char *options_names[] = {
	"--children-pid",
	"--inquiries",
	"--acceptances",
	"--realtime-signals",
	"--child-finished"
};

int print_usage(char *program_name) {
	if (program_name == NULL) {
		return -1;
	}

	printf("Usage: %s"
			"\n\tnumber of children,"
			"\n\tnumber of inquiries,"
			"\n\t--children-pid - print PID of children once they have been created,"
			"\n\t--inquiries - print inquiries from children once they have been received,"
			"\n\t--acceptances - print acceptances sent to children once they have been sent,"
			"\n\t--realtime-signals - print received real-time signals once they have been received,"
			"\n\t--child-finished - print information about children which exited, along with their return code, once they have exited"
			"\n\n", program_name);

	return 0;
}

int parse_args(int argc, char *argv[]) {
	if (argc < 3) {
		print_usage(argv[0]);
		return -1;
	}

	children = strtoul(argv[1], NULL, 10);
	if (children == 0 || children > MAX_CHILDREN) {
		perror("Use a positive number (at most 32) to describe number of children.\n");
		return -2;
	}

	inquiries = strtoul(argv[2], NULL, 10);
	if (inquiries == 0 || inquiries > children) {
		perror("The number of inquiries should be positive and not bigger than children count.\n");
		return -3;
	}
	inquiry_pids = (pid_t *) calloc(children, sizeof(pid_t));
	if (inquiry_pids == NULL) {
		perror("calloc");
		return -4;
	}

	unsigned int arg;
	unsigned int option;
	for (arg = 3; arg < argc; ++arg) {
		for (option = 0; option < sizeof(options_names) / sizeof(*options_names); ++option) {
			if (strcmp(options_names[option], argv[arg]) == 0) {
				settings_storage |= (1 << option);
				break;
			}
		}

		if (option >= sizeof(options_names) / sizeof(*options_names)) {
			fprintf(stderr, "Unrecognized option: %s\n", argv[arg]);
			print_usage(argv[0]);
			return -5;
		}
	}

	return 0;
}

void inquiry_handle(int sig, siginfo_t *siginfo, void *context) {
	if (sig != SIGUSR1) {
		return;
	}

	usleep(1000);
	inquiry_pids[received_inquiries] = siginfo->si_pid;
	++received_inquiries;

	if (IS_ACCEPTANCES_SET(settings_storage)) {
		printf("Received an inquiry number %ld from a child (PID: %d).\n", received_inquiries, siginfo->si_pid);
	}

	if (received_inquiries == inquiries) {
		unsigned int i;
		for (i = 0; i < received_inquiries; ++i) {
			kill(inquiry_pids[i], SIGUSR2);
		}
	} else if (received_inquiries <= children) {
		kill(siginfo->si_pid, SIGUSR2);
	}
}

void sigint_handle(int sig) {
	if (sig != SIGINT) {
		return;
	}
	
	unsigned int i;
	for (i = 0; i < received_inquiries; ++i) {
		kill(inquiry_pids[i], SIGKILL);
	}

	free(inquiry_pids);

	exit(0);
}

void child_handle(int sig, siginfo_t *siginfo, void *context) {
	if (IS_CHILD_FINISHED_SET(settings_storage)) {
		printf("Child (PID: %d) has finished with exit code %d.\n", siginfo->si_pid, siginfo->si_status);
	}

	++children_exited;
}

void sigrt_handle(int sig, siginfo_t *siginfo, void *context) {
	if (sig < SIGRTMIN || sig > SIGRTMAX) {
		return;
	}

	if (IS_REALTIME_SIGNALS_SET(settings_storage)) {
		printf("\tReceived SIGRTMIN + %d from child (PID: %d).\n", (sig - SIGRTMIN), siginfo->si_pid);
	}

	++realtime_signals_received;

}

int parent_configure_signals() {
	struct sigaction inquiry_action;
	memset(&inquiry_action, 0, sizeof(inquiry_action));
	sigemptyset(&inquiry_action.sa_mask);
	inquiry_action.sa_flags = SA_SIGINFO;
	inquiry_action.sa_sigaction = inquiry_handle;
	error_code = sigaction(SIGUSR1, &inquiry_action, NULL);
	if (error_code < 0) {
		perror("Couldn't configure SIGUSR1 for parent.\n");
		return -1;
	}

	struct sigaction sigint_action;
	memset(&sigint_action, 0, sizeof(sigint_action));
	sigemptyset(&sigint_action.sa_mask);
	sigint_action.sa_handler = sigint_handle;
	error_code = sigaction(SIGINT, &sigint_action, NULL);
	if (error_code < 0) {
		perror("Couldn't configure SIGINT for parent.\n");
		return -2;
	}

	struct sigaction sigrt_action;
	memset(&sigrt_action, 0, sizeof(sigrt_action));
	sigemptyset(&sigrt_action.sa_mask);
	sigrt_action.sa_flags = SA_SIGINFO;
	sigrt_action.sa_sigaction = sigrt_handle;
	unsigned int sigrt;
	for (sigrt = SIGRTMIN; sigrt <= SIGRTMAX; ++sigrt) {
		error_code = sigaction(sigrt, &sigrt_action, NULL);
		if (error_code < 0) {
			fprintf(stderr, "Couldn't configure SIGRTMIN + %d for parent.\n", (sigrt - SIGRTMIN));
			return -2;
		}
	}

	struct sigaction child_action;
	memset(&child_action, 0, sizeof(child_action));
	sigemptyset(&child_action.sa_mask);
	child_action.sa_flags = SA_SIGINFO;
	child_action.sa_sigaction = child_handle;
	if (sigaction(SIGCHLD, &child_action, NULL) < 0) {
		perror("Couldn't configure SIGCHLD for parent.\n");
		return -2;
	}

	return 0;
}

void child_response_handle(int sig_no) {
	unsigned int chosen_realtime_signal = rand() % (SIGRTMAX - SIGRTMIN + 1) + SIGRTMIN;
	kill(getppid(), chosen_realtime_signal/*, signal_extra*/);

	exit(sleep_seconds);
}

void child_program() {
	struct sigaction sigerase_action;
	memset(&sigerase_action, 0, sizeof(sigerase_action));
	sigemptyset(&sigerase_action.sa_mask);
	sigerase_action.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sigerase_action, NULL);
	sigaction(SIGCHLD, &sigerase_action, NULL);
	signal(SIGUSR2, child_response_handle);

	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	srand(current_time.tv_usec + ((int) 1E6 * current_time.tv_sec) % 1000);
	sleep_seconds = rand() % (MAX_SLEEP_SECONDS + 1);
	usleep(sleep_seconds * 1E6);
	kill(getppid(), SIGUSR1);

	pause();
}

int create_children() {
	unsigned int child;
	for (child = 0; child < children; ++child) {
		pid_t child_pid = fork();
		if (child_pid < 0) {
			fprintf(stderr, "Couldn't create child number %d.\n", child + 1);
			return -1;
		} else if (child_pid == 0) {
			child_program();
		} else {
			if (IS_CHILDREN_PID_SET(settings_storage)) {
				printf("Parent (PID %d) has just created a child (PID %d).\n", getpid(), child_pid); 
			}
			continue;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	error_code = parse_args(argc, argv);
	if (error_code < 0) {
		perror("Errors occured while parsing arguments, exiting...\n");
		return 1;
	}

	if (parent_configure_signals() < 0) {
		return 2;
	}

	if (create_children() < 0) {
		return 3;
	}

	while (children_exited < children && realtime_signals_received != children);
/*
	if (collect_children_responses() < 0) {
		return 4;
	}
*/
	return 0;
}
