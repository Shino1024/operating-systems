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
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>

#include "fifo.h"

#define MAX_SIZE 64

sem_t* semaphore[MAX_SIZE + 1];
fifo* queue;
int* clients;
int shaves;

void end (int s){
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
	printf("Time: %d.%06ld\n", (int)t.tv_sec, t.tv_nsec);
}

char * cat(char a[], char b[]) {
	char* r = malloc(14);
	strcpy(r, a);
	strcpy(r+10, b);
	return r;
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("You provided a wrong argument to client_factory.\n");
		return 1;
	}
	shaves = atoi(argv[1]);

	char num[4];
	char name[15] = "/semaphore";
	semaphore[0] = sem_open("/semaphore000", O_RDWR, 0777, 0);
	unsigned int i;
	for(i = 1; i < MAX_SIZE + 1; i++) {
		sprintf(num, "%03d", i);
		char *a = cat(name, num);
		semaphore[i] = sem_open(a, O_RDWR, 0777, 1);
		free(a);
	}

	int memory = shm_open("/memory", O_RDWR, 0777);
	ftruncate(memory, MAX_SIZE * sizeof(int) * 2);
	queue = (fifo*)mmap(NULL, MAX_SIZE * sizeof(fifo), PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0);
	
	signal(SIGINT, end);

	int chair_number;
	
	unsigned int s;
	for(s = 0; s < shaves; s++) {
		printf("Client %d enters waiting room. ", getpid());
		print_time();
		chair_number = -1;
		int v;
		sem_getvalue(semaphore[0], &v);
		if(v == 0) {
			fifo_add(queue, getpid(), 0);
			sem_post(semaphore[0]);
			printf("Client %d is being shaved. ", getpid());
			print_time();
			do {
				sem_getvalue(semaphore[0], &v);
			} while (v == 1);
			printf("Client %d was shaved %d times. ", getpid(), s + 1);
			print_time();
		}
		else {
			for (i = 1; i < MAX_SIZE + 1; i++){
				sem_getvalue(semaphore[i], &v);
				if (v == 1) {
					sem_wait(semaphore[i]);
					chair_number = i;
					fifo_add(queue, getpid(), i);
					printf("Client %d sits in waiting room. ", getpid());
					print_time();

					sem_post(semaphore[0]);
					sem_post(semaphore[i]);
					printf("Client %d is being shaved. ", getpid());
					print_time();

					do {
						sem_getvalue(semaphore[0], &v);
					} while (v == 1);

					printf("Client %d was shaved %d times. ", getpid(), s + 1);
					print_time();
					break;
				}
			}
			if (chair_number == -1){
				printf("There is no place for client %d in the waiting room. ", getpid());
				print_time();
			}
		}
		printf("\n");
	}

	end(0);
}
