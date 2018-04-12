#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

typedef enum program_state {
	RUNNING,
	PAUSED,
	DEAD
} program_state;

static program_state state = RUNNING;

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
		printf("SIGTSTP detected - send SIGTSTP (Ctrl+Z) to continue or SIGINT (Ctrl+C) to stop the program.\n");
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

int display_date() {
	time_t current_time_seconds;
	time(&current_time_seconds);
	struct tm *current_time = localtime(&current_time_seconds);
	printf("%s\n", asctime(current_time));

	sleep(1);

	return 0;
}

int main_loop() {
	while (1) {
		switch (state) {
			case RUNNING:
				display_date();
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
	main_loop();

	return 0;
}
