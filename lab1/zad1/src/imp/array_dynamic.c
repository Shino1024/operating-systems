#include <stdlib.h>

#include "../hdr/array_dynamic.h"

array_dynamic make_array_dynamic(unsigned int array_size, unsigned int block_size) {
	array_dynamic ad_ret;
	ad_ret.array_size = array_size;
	ad_ret.block_size = block_size;
	ad_ret.array = (char **) calloc(array_size, sizeof(block));

	unsigned int i;
	for (i = 0; i < array_size; ++i) {
		ad_ret.array[i] = (block) calloc(block_size, sizeof(chunk));
	}

	return ad_ret;
}

int free_array_dynamic(array_dynamic *ad) {
	if (ad.array == NULL) {
		return -1;
	}

	unsigned int i = 0;
	for (i = 0; i < ad.array_size; ++i) {
		free(ad.array[i]);
	}
	free(ad.array);
	ad.array_size = 0;
	ad.block_size = 0;

	return 0;
}

int append_block(array_dynamic *ad) {
	if (ad.array == NULL) {
		return -1;
	}

	++ad->array_size;
	ad->array = (block_array) realloc(ad->array, ad->array_size);
	ad->array[ad->array_size - 1] = (block) calloc(ad->block_size, sizeof(chunk));
	gen_data(ad->array[ad->array_size - 1], ad->block_size);
}

int pop_block(array_dynamic *ad) {
	if (ad.array == NULL) {
		return -1;
	}

	--ad.array_size;
	ad->array
}
