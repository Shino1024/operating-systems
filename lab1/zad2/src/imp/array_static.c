#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../hdr/array_static.h"
#include "../hdr/util.h"

char array_static[MAX_BLOCK_NUMBER][MAX_BLOCK_SIZE];
char alloc_info[MAX_BLOCK_NUMBER];

int make_static_array() {
	return 0;
}

int zero_out_static_array() {
	ZERO(array_static);
	ZERO(alloc_info);
	return 0;
}

int append_block_static(unsigned int index) {
	if (index >= MAX_BLOCK_NUMBER) {
		return -1;
	}

	if (alloc_info[index] != 0) {
		return -2;
	}

	gen_data(array_static[index], MAX_BLOCK_SIZE);
	alloc_info[index] = 1;

	return 0;
}

int pop_block_static(unsigned int index) {
	if (index >= MAX_BLOCK_NUMBER) {
		return -1;
	}

	if (alloc_info[index] == 0) {
		return -2;
	}

	ZERO(array_static[index]);
	alloc_info[index] = 0;

	return 0;
}

unsigned int find_most_matching_block_static(unsigned int index) {
	if (index >= MAX_BLOCK_NUMBER) {
		return -1;
	}

	if (alloc_info[index] == 0) {
		return -2;
	}

	long chosen_sum = 0;
	unsigned int i, j;
	for (j = 0; j < MAX_BLOCK_SIZE; ++j) {
		chosen_sum += (long) array_static[index][j];
	}

	long difference;
	long min_difference = LONG_MAX;
	int min_index = -3;
	long sum;
	for (i = 0; i < MAX_BLOCK_NUMBER; ++i) {
		if (i == index) {
			continue;
		}

		if (alloc_info[i] == 0) {
			continue;
		}

		sum = 0;
		for (j = 0; j < MAX_BLOCK_SIZE; ++j) {
			sum += (long) array_static[i][j];
		}
		
		difference = ABS(chosen_sum - sum);
		if (difference < min_difference) {
			min_difference = difference;
			min_index = i;
		}
	}

	return min_index;
}

