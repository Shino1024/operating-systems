#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

typedef enum program_state {
	RUNNING,
	PAUSED,
	DEAD
} program_state;

static program_state state = RUNNING;

static char *date_batch = "./date_batch.sh";

static pid_t child_pid;

static int error_code;

void sigint_handle(int signal) {
	if (signal != SIGINT) {
		return;
	}

	printf("SIGINT has been received.\n");

	state = DEAD;
}

void sigtstp_handle(int signal) {
	if (signal != SIGTSTP) {
		return;
	}

	if (state == RUNNING) {
		if (child_pid == 0) {
			return;
		}

		kill(child_pid, SIGINT);
		printf("SIGTSTP detected - send SIGTSTP (Ctrl+Z) to continue or SIGINT (Ctrl+C) to stop the program.\n");
		child_pid = 0;
		state = PAUSED;
	} else if (state == PAUSED) {
		state = RUNNING;
	}
}

int configure_signals() {
	signal(SIGINT, sigint_handle);

	struct sigaction sigtstp_action;
	sigemptyset(&(sigtstp_action.sa_mask));
	sigtstp_action.sa_handler = sigtstp_handle;
	sigtstp_action.sa_flags = 0;
	sigaction(SIGTSTP, &sigtstp_action, NULL);

	return 0;
}

int unconfigure_signals() {
	signal(SIGINT, SIG_DFL);

	struct sigaction sigtstp_action;
	sigemptyset(&(sigtstp_action.sa_mask));
	sigtstp_action.sa_handler = SIG_DFL;
	sigtstp_action.sa_flags = 0;
	sigaction(SIGTSTP, &sigtstp_action, NULL);

	return 0;
}


int display_date_in_subprocess() {
	child_pid = fork();
	if (child_pid < 0) {
		perror("Couldn't create a child process.\n");
		return -1;
	} else if (child_pid == 0) {
		unconfigure_signals();
		execlp(date_batch, date_batch, NULL);
		printf("Error! Subprocess %d exited abruptly.\n\n", getpid());
		exit(1);
	} else {
		wait(NULL);
	}

	return 0;
}

int main_loop() {
	while (1) {
		switch (state) {
			case RUNNING:
				display_date_in_subprocess();
				break;

			case PAUSED:
				pause();
				break;
			
			case DEAD:
				exit(0);

			default:
				return -1;
		}
	}

	return 0;
}

int main(int argc, char *argv[]) {
	configure_signals();
	error_code = main_loop();
	if (error_code < 0) {
		perror("Errors occured. Exiting...\n");
		return 1;
	}

	return 0;
}
