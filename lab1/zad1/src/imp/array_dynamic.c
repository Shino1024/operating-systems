#include <stdlib.h>
#include <limits.h>

#include "../hdr/array_dynamic.h"

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

int append_block_dynamic(array_dynamic *ad, unsigned int i) {
	if (ad->array == NULL) {
		return -1;
	}

	if (ad->array_size <= i) {
		return -2;
	}

	if (ad->array[i] != NULL) {
		return -3;
	}

	ad->array[i] = (block) calloc(ad->block_size, sizeof(chunk));
	gen_data(ad->array[i], ad->block_size);

	return 0;
}

int pop_block_dynamic(array_dynamic *ad, unsigned int i) {
	if (ad->array == NULL) {
		return -1;
	}

	if (ad->array_size <= i) {
		return -2;
	}

	if (ad->array[i] == NULL) {
		return -3;
	}

	free(ad->array[i]);
	ad->array[i] = NULL;

	return 0;
}

int find_most_matching_element_dynamic(array_dynamic *ad) {
	if (ad->array == NULL) {
		return -1;
	}

	long difference;
	long min_difference = LONG_MAX;
	int min_indice = 0;
	unsigned int i, j;
	long sum = 0;
	for (i = 0; i < ad->array_size; ++i) {
		if (ad->array[i] == NULL) {
			continue;
		}

		for (j = 0; j < ad->block_size; ++j) {
			sum += (long) ad->array[i][j];
		}

		difference = ABS(sum - i);
		if (difference < min_difference) {
			min_difference = difference;
			min_indice = i;
		}

		sum = 0;
	}

	return min_indice;
}
