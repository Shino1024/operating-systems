#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../hdr/array_static.h"
#include "../hdr/util.h"

char array_static[MAX_BLOCK_NUMBER][MAX_BLOCK_SIZE];
int size;

void zero_out_static_array() {
	size = 0;
	ZERO(array_static);	
}

int append_block() {
	if (size >= MAX_BLOCK_NUMBER) {
		return -1;
	}

	gen_data(array_static[size], MAX_BLOCK_SIZE);
	++size;

	return 0;
}

int pop_block() {
	if (size <= 0) {
		return -1;
	}

	--size;
	ZERO(array_static[size]);

	return 0;
}

int find_most_matching_element() {
	if (size <= 0) {
		return -1;
	}

	long difference;
	long min_difference = LONG_MAX;
	int min_indice = 0;
	unsigned int i, j;
	long sum = 0;
	for (i = 0; i < size; ++i) {
		for (j = 0; j < MAX_BLOCK_SIZE; ++j) {
			sum += (long) array_static[i][j];
		}
		
		difference = ABS(sum - i);
		if (difference < min_difference) {
			min_difference = difference;
			min_indice = i;
		}
		printf("%d, %ld, %ld\n", i, sum, difference);
		sum = 0;
	}

	return min_indice;
}

