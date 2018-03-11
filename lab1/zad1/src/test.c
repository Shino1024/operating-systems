#include <stdio.h>
#include <stdlib.h>

#include "hdr/typedefs.h"

// included only for testing
#include "imp/array_static.c"

#include "imp/array_dynamic.c"

#include "imp/util.c"

int main(int argc, char *argv[]) {
	append_block(4);
	append_block(5);
	append_block(6);

	pop_block(4);
	pop_block(7);
	pop_block(9);
	pop_block(5);
	pop_block(7);
	pop_block(8);
	pop_block(4);
	
	append_block(4);
append_block(6);
append_block(5);
append_block(8);
append_block(9);
append_block(11);
append_block(11111);
append_block(4);
append_block(0);

zero_out_static_array();


	printf("after\n");
	printf("%d\n", find_most_matching_element(4));


	array_dynamic *ad = make_array_dynamic(120, 150);
	unsigned int i;
	for (i = 2; i < ad->array_size + 8; i += 3) {
		append_block_dynamic(ad, i);
	}

	for (i = 3; i < ad->array_size + 11; i += 2) {
		pop_block_dynamic(ad, i);
	}

	printf("Most Matching Block Dynamic: %d\n", find_most_matching_block_dynamic(ad, 8));

	free_array_dynamic(&ad);

	return 0;
}
