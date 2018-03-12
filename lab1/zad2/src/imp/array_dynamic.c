#include <stdlib.h>
#include <limits.h>

#include "../hdr/array_dynamic.h"
#include "../hdr/util.h"

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
	if (*ad == NULL) {
		return -1;
	}
	if ((*ad)->array == NULL) {
		return -2;
	}

	unsigned int i = 0;
	for (i = 0; i < (*ad)->array_size; ++i) {
		if ((*ad)->array[i] != NULL) {
			free((*ad)->array[i]);
		}
	}
	free((*ad)->array);
	*ad = NULL;

	return 0;
}

int append_block_dynamic(array_dynamic *ad, unsigned int index) {
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
