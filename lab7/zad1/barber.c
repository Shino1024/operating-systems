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

int semaphore;
int memory;
fifo *queue;
int chairs;

struct timespec get_time() {
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time;
}

void print_time() {
	struct timespec t = get_time();
	printf("Time: %d.%06ld\n", (int)t.tv_sec, t.tv_nsec);
}

void sem_up(int num) {
	struct sembuf sb;
	sb.sem_num = num;
	sb.sem_op = 1;
	semop(semaphore, &sb, 1);
}

void sem_down(int num) {
	struct sembuf sb;
	sb.sem_num = num;
	sb.sem_op = -1;
	semop(semaphore, &sb, 1);
}

void end(int s) {
	printf("Barber EXIT.\n");
	semctl(semaphore, 0, IPC_RMID, 0);
	shmdt(queue);
	shmctl(memory, IPC_RMID, NULL);
	exit(1);
}

void end_atexit() {
	printf("Barber EXIT.\n");
	semctl(semaphore, 0, IPC_RMID, 0);
	shmdt(queue);
	shmctl(memory, IPC_RMID, NULL);
	exit(1);
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("Usage: %s number_of_seats\n", argv[0]);
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
	
	chairs = atoi(argv[1]);
	semaphore = semget(ftok(getenv("HOME"), KEY), MAX_SIZE + 1, IPC_CREAT | 0777);
	memory = shmget(ftok(getenv("HOME"), KEY), MAX_SIZE * sizeof(fifo), IPC_CREAT | 0777);
	queue = (fifo*)shmat(memory, NULL, 0);
	
	fifo_init(queue);
	
	union semun {
		int val;
		struct semid_ds *buf;
		unsigned short *array;
	} a;
	a.val = 0;
	semctl(semaphore, 0, SETVAL, a);
	a.val = 1;
	unsigned int i;
	for (i = 1; i < MAX_SIZE + 1; i++) semctl(semaphore, i, SETVAL, a);
	a.val = 0;
	for (i = chairs; i < MAX_SIZE + 1; i++) semctl(semaphore, i, SETVAL, a); //unused chairs
	
	fifo data;

	printf("Barber starts work. ");	
	print_time();
	printf("\n");

	for (;;) {
		printf("Barber goes to sleep. ");
		print_time();
		while (semctl(semaphore, 0, GETVAL) == 0);

		data = fifo_get(queue);
		printf("Client %d wakes the barber up. " , data.pid);
		print_time();
		printf("Barber starts shaving client %d. ", data.pid);
		print_time();

		sem_down(0);
		printf("Barber ends shaving client %d. ", data.pid);
		print_time();
		printf("\n");
	}
	return 0;
}
