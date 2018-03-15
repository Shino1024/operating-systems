#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/time.h>

#include "condensed_lib.h"

// UTIL
void gen_data(char *blk, unsigned int block_size) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand((unsigned int) ((tv.tv_sec % 1000) * 1E6 + tv.tv_usec));
	unsigned int i;
	for (i = 0; i < block_size; ++i) {
		blk[i] = rand() % (CHAR_MAX + 1);
	}
}

// STATIC
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

// DYNAMIC
array_dynamic * make_array_dynamic(unsigned int array_size, unsigned int block_size) {
	array_dynamic *ad_ret = (array_dynamic *) calloc(1, sizeof(array_dynamic));
	if (ad_ret == NULL) {
		return NULL;
	}

	ad_ret->array_size = array_size;
	ad_ret->block_size = block_size;
	ad_ret->array = (block_array) calloc(array_size, sizeof(block));

	return ad_ret;
}

int free_array_dynamic(array_dynamic **ad) {
	if (ad == NULL) {
		return -1;
	}

	if (*ad == NULL) {
		return -2;
	}

	if ((*ad)->array == NULL) {
		return -3;
	}

	unsigned int i = 0;
	for (i = 0; i < (*ad)->array_size; ++i) {
		if ((*ad)->array[i] != NULL) {
			free((*ad)->array[i]);
		}
	}
	free((*ad)->array);
	free(*ad);
	*ad = NULL;

	return 0;
}

int append_block_dynamic(array_dynamic *ad, unsigned int index, const char *data) {
	if (ad == NULL) {
		return -1;
	}

	if (ad->array == NULL) {
		return -2;
	}

	if (ad->array_size <= index) {
		return -3;
	}

	if (ad->array[index] != NULL) {
		return -4;
	}

	ad->array[index] = (block) calloc(ad->block_size, sizeof(chunk));
	unsigned int data_length = strlen(data);
	if (data_length >= ad->block_size) {
		memcpy(ad->array[index], data, ad->block_size - 1);
	} else {
		memcpy(ad->array[index], data, data_length - 1);
	}

	return 0;
}

int append_block_gen_dynamic(array_dynamic *ad, unsigned int index) {
	if (ad == NULL) {
		return -1;
	}

	if (ad->array == NULL) {
		return -2;
	}

	if (ad->array_size <= index) {
		return -3;
	}

	if (ad->array[index] != NULL) {
		return -4;
	}

	ad->array[index] = (block) calloc(ad->block_size, sizeof(chunk));
	gen_data(ad->array[index], ad->block_size);

	return 0;
}

int pop_block_dynamic(array_dynamic *ad, unsigned int index) {
	if (ad == NULL) {
		return -1;
	}

	if (ad->array == NULL) {
		return -2;
	}

	if (ad->array_size <= index) {
		return -3;
	}

	if (ad->array[index] == NULL) {
		return -4;
	}

	free(ad->array[index]);
	ad->array[index] = NULL;

	return 0;
}

unsigned int find_most_matching_block_dynamic(array_dynamic *ad, unsigned int index) {
	if (ad == NULL) {
		return -1;
	}

	if (ad->array == NULL) {
		return -2;
	}

	if (index >= ad->array_size) {
		return -3;
	}

	if (ad->array[index] == NULL) {
		return -4;
	}

	long chosen_sum = 0;
	unsigned int i, j;
	for (j = 0; j < ad->block_size; ++j) {
		chosen_sum += (long) ad->array[index][j];
	}

	long difference;
	long min_difference = LONG_MAX;
	int min_index = -5;
	long sum;
	for (i = 0; i < ad->array_size; ++i) {
		if (i == index) {
			continue;
		}

		if (ad->array[i] == NULL) {
			continue;
		}

		sum = 0;
		for (j = 0; j < ad->block_size; ++j) {
			sum += (long) ad->array[i][j];
		}
		difference = ABS(chosen_sum - sum);
		if (difference < min_difference) {
			min_difference = difference;
			min_index = i;
		}
	}

	return min_index;
}
