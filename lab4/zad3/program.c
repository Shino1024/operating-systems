#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <string.h>

volatile unsigned int signals_sent;
volatile unsigned int signals_received_by_child;
volatile unsigned int signals_received_by_parent;

static pid_t child_pid;

struct {
    unsigned long signal_number;
    unsigned long type;
} program_params;

int print_usage(char *program_name) {
    printf("Usage: %s"
    "\n\tnumber of signals to be sent to the child process,"
           "\n\tmethod of signaling (3 available)"
					 "\n\n", program_name);

    return 0;
}

int parse_args(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
				return -1;
    }

    program_params.signal_number = strtoul(argv[1], NULL, 10);
    if (program_params.signal_number == 0 || program_params.signal_number > UINT_MAX) {
        fprintf(stderr, "Number of signals should be a positive integer, not bigger than %d.\n", UINT_MAX);
        return -2;
    }

    program_params.type = strtoul(argv[2], NULL, 10);
    if (program_params.type < 1 || program_params.type > 3) {
        perror("Type should be 1, 2 or 3.\n");
        return -3;
    }

    return 0;
}

int child_block_signals() {
    sigset_t block_set;
    sigfillset(&block_set);
		if (program_params.type == 1 || program_params.type == 2) {
    	sigdelset(&block_set, SIGUSR1);
    	sigdelset(&block_set, SIGUSR2);
		} else if (program_params.type == 3) {
    	sigdelset(&block_set, SIGRTMIN);
    	sigdelset(&block_set, SIGRTMAX);
		}

    if (sigprocmask(SIG_BLOCK, &block_set, NULL) < 0) {
        perror("sigprocmask");
        return -1;
    }

    return 0;
}

void child1_1_handle(int sig) {
    printf("Caught SIGUSR1 from parent. Responding...\n");
		kill(getppid(), SIGUSR1);
    ++signals_received_by_child;
}

void child1_2_handle(int sig) {
    printf("Caught SIGUSR2 from parent, received %d signals. Exiting...\n", signals_received_by_child);
    exit(0);
}

void child2_1_handle(int sig) {
    printf("Caught SIGRTMIN from parent. Responding...\n");
		kill(getppid(), SIGRTMIN);
    ++signals_received_by_child;
}

void child2_2_handle(int sig) {
    printf("Caught SIGRTMAX from parent, received %d signals. Exiting...\n", signals_received_by_child);
    exit(0);
}

void parent1_1_handle(int sig) {
    printf("Caught SIGUSR1 from child.\n");
    ++signals_received_by_parent;
}

void parent1_2_handle(int sig) {
	printf("Caught SIGUSR1 from child.\n");
	++signals_received_by_parent;
}

void sigint_handle(int sig) {
    printf("Caught SIGINT, sending SIGKILL to child...\n");
    kill(child_pid, SIGKILL);
}

int configure_child_signals() {
	if (program_params.type == 1 || program_params.type == 2) {
    struct sigaction sigusr1_action;
    memset(&sigusr1_action, 0, sizeof(sigusr1_action));
    sigemptyset(&sigusr1_action.sa_mask);
    sigusr1_action.sa_handler = child1_1_handle;
    if (sigaction(SIGUSR1, &sigusr1_action, NULL) < 0) {
        perror("sigaction");
        return -1;
    }
    struct sigaction sigusr2_action;
    memset(&sigusr2_action, 0, sizeof(sigusr2_action));
    sigemptyset(&sigusr2_action.sa_mask);
    sigusr2_action.sa_handler = child1_2_handle;
    if (sigaction(SIGUSR2, &sigusr2_action, NULL) < 0) {
        perror("sigaction");
        return -1;
    }
	} else if (program_params.type == 3) {
    struct sigaction sigrtmin_action;
    memset(&sigrtmin_action, 0, sizeof(sigrtmin_action));
    sigemptyset(&sigrtmin_action.sa_mask);
    sigrtmin_action.sa_handler = child2_1_handle;
    if (sigaction(SIGRTMIN, &sigrtmin_action, NULL) < 0) {
        perror("sigaction");
        return -1;
    }
    struct sigaction sigrtmax_action;
    memset(&sigrtmax_action, 0, sizeof(sigrtmax_action));
    sigemptyset(&sigrtmax_action.sa_mask);
    sigrtmax_action.sa_handler = child2_2_handle;
    if (sigaction(SIGRTMAX, &sigrtmax_action, NULL) < 0) {
        perror("sigaction");
        return -1;
    }
	}

    return 0;
}

int configure_parent_signals() {
	if (program_params.type == 1 || program_params.type == 2) {
    struct sigaction sigusr1_action;
    memset(&sigusr1_action, 0, sizeof(sigusr1_action));
    sigemptyset(&sigusr1_action.sa_mask);
		if (program_params.type == 1) {
    	sigusr1_action.sa_handler = parent1_1_handle;
		} else if (program_params.type == 2) {
			sigusr1_action.sa_handler = parent1_2_handle;
		}
    if (sigaction(SIGUSR1, &sigusr1_action, NULL) < 0) {
        perror("sigaction");
        return -1;
    }
	} else if (program_params.type == 3) {
    struct sigaction sigrtmin_action;
    memset(&sigrtmin_action, 0, sizeof(sigrtmin_action));
    sigemptyset(&sigrtmin_action.sa_mask);
		if (program_params.type == 1) {
    	sigrtmin_action.sa_handler = parent1_1_handle;
		} else if (program_params.type == 2) {
			sigrtmin_action.sa_handler = parent1_2_handle;
		}
    if (sigaction(SIGRTMIN, &sigrtmin_action, NULL) < 0) {
        perror("sigaction");
        return -1;
    }
	}

    struct sigaction sigint_action;
    memset(&sigint_action, 0, sizeof(sigint_action));
    sigemptyset(&sigint_action.sa_mask);
    sigint_action.sa_handler = sigint_handle;
    if (sigaction(SIGINT, &sigint_action, NULL) < 0) {
        perror("sigaction");
        return -1;
    }

    return 0;
}

int send_signals() {
	unsigned int counter;
    switch (program_params.type) {
        case 1:
            for (counter = 0; counter < program_params.signal_number; ++counter) {
                printf("Sending SIGUSR1 to child...\n");
               if (kill(child_pid, SIGUSR1) != 0) {
								perror("Couldn't send SIGUSR1 to child...\n");
							}
								sleep(1);
                ++signals_sent;
            }

            printf("Sending SIGUSR2 to child...\n");
            if (kill(child_pid, SIGUSR2) != 0) {
								perror("Couldn't send SIGUSR2 to child...\n");
							}
            break;

        case 2:
						for (counter = 0; counter < program_params.signal_number; ++counter) {
							printf("Sending SIGUSR1 to child...\n");
							if (kill(child_pid, SIGUSR1) != 0) {
								perror("Couldn't send SIGUSR1 to child...\n");
							}
							pause();
							++signals_sent;
						}

						printf("Sending SIGUSR2 to child...\n");
						if (kill(child_pid, SIGUSR2) != 0) {
							perror("Couldn't send SIGUSR2 to child...\n");
						}
            break;

        case 3:
						for (counter = 0; counter < program_params.signal_number; ++counter) {
							printf("Sending SIGRTMIN to child...\n");
							if (kill(child_pid, SIGRTMIN) != 0) {
								perror("Couldn't send SIGRTMIN to child...\n");
							}
							sleep(1);
							++signals_sent;
						}

						printf("Sending SIGRTMAX to child...\n");
						if (kill(child_pid, SIGRTMAX) != 0) {
							perror("Couldn't send SIGRTMAX to child...\n");
						}
            break;

        default:
            return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (parse_args(argc, argv) < 0) {
        return 1;
    }

    child_pid = fork();
    if (child_pid == -1) {
        perror("Couldn't create a child process.\n");
        return 2;
    } else if (child_pid == 0) {
        if (configure_child_signals() < 0) {
            perror("Couldn't configure signals for child.\n");
            exit(1);
        }

				while (1) {
					pause();
				}
/*
        if (child_block_signals() < 0) {
            perror("Couldn't configure blocked signals for child.\n");
            exit(2);
        }*/
    } else if (child_pid > 0) {
        if (configure_parent_signals()) {
            perror("Couldn't configure signals for parent.\n");
            kill(child_pid, SIGKILL);
            exit(1);
        }

        usleep(100000);
        send_signals();

        printf("Signals sent: %d\n"
        "Signals received by parent: %d\n",
        signals_sent, signals_received_by_parent);
    }

    return 0;
}
