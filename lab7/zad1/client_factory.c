#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>

#include "fifo.h"

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s number_of_clients shaves_per_client\n", argv[0]);
		return 1;
	}
	int C = atoi(argv[1]);
	int pid;
	while (C--) {
		pid = fork();
		if (pid == 0) execlp("./client", "./client", argv[2], (char *) NULL);
	}
	return 0;
}