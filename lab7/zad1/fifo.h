#ifndef FIFO_H
#define FIFO_H

#define MAX_SIZE 64
#define KEY 147

typedef struct fifo {
	int num;
	int pid;
} fifo;

fifo fifo_get(fifo*);

void fifo_init(fifo*);

void fifo_add(fifo*, int, int);

int fifo_full(fifo*);

int fifo_size(fifo*);

#endif
