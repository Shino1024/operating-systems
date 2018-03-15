#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "../hdr/array_static.h"
#include "../hdr/util.h"

char array_static[MAX_BLOCK_NUMBER][MAX_BLOCK_SIZE];
char alloc_info[MAX_BLOCK_NUMBER];
unsigned int array_size_static = MAX_BLOCK_NUMBER;
unsigned int block_size_static = MAX_BLOCK_SIZE;

int make_static_array(unsigned int array_size, unsigned int block_size) {
	if (array_size >= MAX_BLOCK_NUMBER) {
		array_size_static = MAX_BLOCK_NUMBER;
	} else {
		array_size_static = array_size;
	}

	if (block_size >= MAX_BLOCK_SIZE) {
		block_size_static = MAX_BLOCK_SIZE;
	} else {
		block_size_static = block_size;
	}

	return 0;
}

int zero_out_static_array() {
	ZERO(array_static);
	ZERO(alloc_info);
	array_size_static = 0;
	block_size_static = 0;
	return 0;
}

int append_block_static(unsigned int index, const char *data) {
	if (index >= array_size_static) {
		return -1;
	}

	if (alloc_info[index] != 0) {
		return -2;
	}

	unsigned int data_length = strlen(data);
	if (data_length >= block_size_static) {
		memcpy(array_static[index], data, block_size_static - 1);
	} else {
		memcpy(array_static[index], data, data_length - 1);
	}
	alloc_info[index] = 1;

	return 0;
}

int append_block_gen_static(unsigned int index) {
	if (index >= array_size_static) {
		return -1;
	}

	if (alloc_info[index] != 0) {
		return -2;
	}

	gen_data(array_static[index], block_size_static);
	alloc_info[index] = 1;

	return 0;
}

int pop_block_static(unsigned int index) {
	if (index >= array_size_static) {
		return -1;
	}

	if (alloc_info[index] == 0) {
		return -2;
	}

	ZERO(array_static[index]);
	alloc_info[index] = 0;

	return 0;
}

int find_most_matching_block_static(unsigned int index) {
	if (index >= array_size_static) {
			return -1;
	}

	if (alloc_info[index] == 0) {
		return -2;
	}

	long chosen_sum = 0;
	unsigned int i, j;
	for (j = 0; j < block_size_static; ++j) {
		chosen_sum += (long) array_static[index][j];
	}

	long difference;
	long min_difference = LONG_MAX;
	int min_index = -3;
	long sum;
	for (i = 0; i < array_size_static; ++i) {
		if (i == index) {
			continue;
		}

		if (alloc_info[i] == 0) {
			continue;
		}

		sum = 0;
		for (j = 0; j < block_size_static; ++j) {
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

