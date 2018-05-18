#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>

#include "fifo.h"

sem_t* semaphore[MAX_SIZE + 1];
fifo* queue;
int chairs;
unsigned int i;

struct timespec get_time() {
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time;
}

void print_time() {
	struct timespec t = get_time();
	printf("Time: %d.%06ld\n", (int) t.tv_sec, t.tv_nsec);
}

void end(int s) {
	printf("Barber EXIT.\n");
	munmap(queue, MAX_SIZE * sizeof(int) * 2);
	shm_unlink("/memory");
	for(i = 0; i < MAX_SIZE + 1; ++i) {
		sem_close(semaphore[i]);
	}
	exit(1);
}

void end_atexit() {
	printf("Barber EXIT.\n");
	munmap(queue, MAX_SIZE * sizeof(int) * 2);
	shm_unlink("/memory");
	for(i = 0; i < MAX_SIZE + 1; ++i) {
		sem_close(semaphore[i]);
	}
	exit(1);
}

char * cat(char a[], char b[]) {
	char *r = (char *) malloc(14);
	if (r == NULL) {
		exit(3);
	}
	strcpy(r, a);
	strcpy(r + 10, b);
	return r;
}


int main(int argc, char *argv[]){
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
	char num[4];
	char name[15] = "/semaphore";
	semaphore[0] = sem_open("/semaphore000", O_CREAT, 0777, 0);

	for (i = 1; i < MAX_SIZE + 1; ++i) {
		sprintf(num, "%03d", i);
		char *a = cat(name, num);
		semaphore[i] = sem_open(a, O_CREAT, 0777, 1);
		free(a);
	}
	
	int memory = shm_open("/memory", O_CREAT | O_RDWR, 0777);
	ftruncate(memory, (MAX_SIZE + 2) * sizeof(fifo));
	queue = (fifo *) mmap(NULL, (MAX_SIZE + 2) * sizeof(fifo), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, memory, 0);
	fifo_init(queue);
	
	int val;
	fifo data;

	printf("Barber starts work. ");	
	print_time();
	printf("\n");

	for (;;) {
		printf("Barber goes to sleep. ");
		print_time();
		do {
			sem_getvalue(semaphore[0], &val);
		} while(val == 0);

		data = fifo_get(queue);
		printf("Client %d wakes the barber up. " , data.pid);
		print_time();
		printf("Barber starts shaving client %d. ", data.pid);
		print_time();
		sem_wait(semaphore[0]);
		printf("Barber ends shaving client %d. ", data.pid);
		print_time();
		printf("\n");
	}
	return 0;
}
