#include <stdio.h>
#include <stdlib.h>

#include "condensed_lib.c"

int main(int argc, char *argv[]) {
	make_static_array(555, 555);
	append_block_gen_static(4);
	zero_out_static_array();
	make_static_array(555, 555);
	append_block_gen_static(5);
	append_block_gen_static(6);

	pop_block_static(4);
	pop_block_static(7);
	pop_block_static(9);
	//pop_block_static(5);
	pop_block_static(7);
	pop_block_static(8);
	pop_block_static(4);
	
	append_block_gen_static(4);
append_block_gen_static(6);
append_block_gen_static(5);
append_block_gen_static(8);
append_block_gen_static(9);
append_block_gen_static(11);
append_block_gen_static(11111);
append_block_gen_static(4);
append_block_gen_static(0);

//zero_out_static_array();


	printf("after\n");
	printf("%d\n", find_most_matching_block_static(4));


	array_dynamic *ad = make_array_dynamic(120, 150);
	unsigned int i;
	for (i = 2; i < ad->array_size + 8; i += 3) {
		append_block_gen_dynamic(ad, i);
	}

	for (i = 3; i < ad->array_size + 11; i += 2) {
		pop_block_dynamic(ad, i);
	}

	printf("Most Matching Block Dynamic: %d\n", find_most_matching_block_dynamic(ad, 8));

	free_array_dynamic(&ad);

	return 0;
}
