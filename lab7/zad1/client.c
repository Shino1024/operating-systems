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
#include <sys/wait.h>
#include "fifo.h"

int semaphore;
int memory;
fifo *queue;
int *clients;
int shaves;
int client_number;

void end(int s) {
	printf("Client %d ended work. EXIT\n", getpid());
	shmdt(queue);
	exit(0);
}

void end_atexit() {
	printf("Client %d ended work. EXIT\n", getpid());
	shmdt(queue);
	exit(0);
}

struct timespec get_time() {
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time;
}

void print_time() {
	struct timespec t = get_time();
	printf("Time: %d.%06ld\n", (int) t.tv_sec, t.tv_nsec);
}

void sem_take(int num) {
	struct sembuf sb;
	sb.sem_num = num;
	sb.sem_op = 1;
	semop(semaphore, &sb, 1);
}

void sem_give(int num) {
	struct sembuf sb;
	sb.sem_num = num;
	sb.sem_op = -1;
	semop(semaphore, &sb, 1);
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("You provided a wrong argument to client_factory.\n");
		return 1;
	}
	if (atexit(end_atexit) != 0) {
		perror("atexit");
		return 2;
	}
	if (signal(SIGINT, end) == SIG_ERR) {
		perror("signal");
		return 2;
	}
	if (signal(SIGTERM, end) == SIG_ERR) {
		perror("signal");
		return 2;
	}

	shaves = atoi(argv[1]);

	semaphore = semget(ftok(getenv("HOME"), KEY), MAX_SIZE + 1, 0);
	memory = shmget(ftok(getenv("HOME"), KEY), MAX_SIZE * sizeof(fifo), 0);
	queue = (fifo *) shmat(memory, NULL, 0);
	
	int chair_number;
	unsigned int s;
	unsigned int i;
	
	for (s = 0; s < shaves; ++s) {
		printf("Client %d enters waiting room. ", getpid());
		print_time();
		chair_number = -1;

		if (semctl(semaphore, 0, GETVAL) == 0) {
			fifo_add(queue, getpid(), 0);
			sem_take(0);
			printf("Client %d is being shaved. ", getpid());
			print_time();

			while (semctl(semaphore, 0, GETVAL) == 1);
			printf("Client %d was shaved %d times. ", getpid(), s + 1);
			print_time();
		} else {
			for (i = 1; i < MAX_SIZE + 1; i++) {
				if (semctl(semaphore, i, GETVAL) == 1) {
					sem_give(i);
					chair_number = i;
					fifo_add(queue, getpid(), i);
					printf("Client %d sits in waiting room. ", getpid());
					print_time();

					sem_take(0);
					sem_take(i);
					printf("Client %d is being shaved. ", getpid());
					print_time();

					while (semctl(semaphore, 0, GETVAL) == 1);
					printf("Client %d was shaved %d times. ", getpid(), s + 1);
					print_time();
					break;
				}
			}
			if (chair_number == -1) {
				printf("There is no place for client %d in the waiting room. ", getpid());
				print_time();
			}
		}
		printf("\n");
	}

	end(0);

	return 0;
}
