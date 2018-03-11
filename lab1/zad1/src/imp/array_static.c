#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../hdr/array_static.h"
#include "../hdr/util.h"

void zero_out_static_array() {
	size = 0;
	memset(array_static, 0, sizeof(array_static));
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
	memset(array_static[size], 0, MAX_BLOCK_SIZE);

	return 0;
}

int find_most_matching_element() {
	if (size <= 0) {
		return -1;
	}

	long matches_differences[size];
	memset(matches_differences, 0, sizeof(matches_differences));
	unsigned int i, j;
	long sum = 0;
	for (i = 0; i < size; ++i) {
		for (j = 0; j < MAX_BLOCK_SIZE; ++j) {
			sum += (long) array_static[i][j];
		}
		
		matches_differences[i] = ABS(sum - i);
		printf("%d, %ld, %ld\n", i, sum, matches_differences[i]);
		sum = 0;
	}

	long min_difference = LONG_MAX;
	int min_indice;
	for (i = 0; i < size; ++i) {
		if (matches_differences[i] < min_difference) {
			min_difference = matches_differences[i];
			min_indice = i;
		}
	}

	return min_indice;
}

