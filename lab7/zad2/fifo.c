#include "fifo.h"

void fifo_init(fifo* f){
	for(int i = 0; i < MAX_SIZE; i++) f[i].pid = f[i].num = -1;
}

void fifo_add(fifo* f, int pid, int num){
	for(int i = 0; i < MAX_SIZE; i++){
		if(f[i].pid == -1){
			f[i].pid = pid;
			f[i].num = num;
			return;
		}
	}
}

void fifo_remove_first(fifo* f){
	for(int i = 0; i < MAX_SIZE - 1; i++) {
		if(f[i].pid == -1 || f[i].pid == -2) return;
		f[i].pid = f[i+1].pid;
		f[i].num = f[i+1].num;
	}
	f[MAX_SIZE].pid = f[MAX_SIZE].num = -1;
}

fifo fifo_get(fifo* f){
	if(f[0].pid == -1) return f[0];
	fifo r;
	r.pid = f[0].pid;
	r.num = f[0].num;
	fifo_remove_first(f);
	return r;
}
