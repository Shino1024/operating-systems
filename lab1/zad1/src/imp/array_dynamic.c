#include <stdlib.h>
#include <limits.h>

#include "../hdr/array_dynamic.h"

array_dynamic make_array_dynamic(unsigned int array_size, unsigned int block_size) {
	array_dynamic ad_ret;
	ad_ret.array_size = array_size;
	ad_ret.block_size = block_size;
	ad_ret.array = (block_array) calloc(array_size, sizeof(block));

	unsigned int i;
	for (i = 0; i < array_size; ++i) {
		ad_ret.array[i] = (block) calloc(block_size, sizeof(chunk));
	}

	return ad_ret;
}

int free_array_dynamic(array_dynamic *ad) {
	if (ad->array_size <= 0 || ad->array == NULL) {
		return -1;
	}

	unsigned int i = 0;
	for (i = 0; i < ad->array_size; ++i) {
		free(ad->array[i]);
	}
	free(ad->array);
	ad->array = NULL;
	ad.array_size = 0;
	ad.block_size = 0;

	return 0;
}

int append_block(array_dynamic *ad) {
	if (ad->array == NULL) {
		return -1;
	}

	++ad->array_size;
	ad->array = (block_array) realloc(ad->array, ad->array_size);
	ad->array[ad->array_size - 1] = (block) calloc(ad->block_size, sizeof(chunk));
	gen_data(ad->array[ad->array_size - 1], ad->block_size);

	return 0;
}

int pop_block(array_dynamic *ad) {
	if (ad->array_size <= 0 || ad->array == NULL) {
		return -1;
	}

	--ad->array_size;
	free(ad->array[ad->array_size]);
	ad->array = (block_array) realloc(ad->array, ad->array_size);

	return 0;
}

int find_most_matching_elementi(array_dynamic *ad) {
	if (ad->array_size <= 0 || ad->array == NULL) {
		return -1;
	}

	long difference;
	long min_difference = LONG_MAX;
	int min_indice = 0;
	unsigned int i, j;
	long sum = 0;
	for (i = 0; i < ad->array_size; ++i) {
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
