#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

int gen_record(record dest, unsigned int size) {
	if (dest == NULL) {
		return -1;
	}

	struct timeval rand_tv;
	gettimeofday(&rand_tv, NULL);
	srand48(rand_tv.tv_usec + (rand_tv.tv_sec % 1000) * 1E6);
	
	unsigned int i;
	for (i = 0; i < size; ++i) {
		dest[i] = (unsigned char) (lrand48() % 94 + 33);
	}

	return 0;
}

int insertion_sort(dataset data, unsigned int data_size, unsigned int record_size) {
	return 0;
}
