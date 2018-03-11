#include <stdio.h>
#include <stdlib.h>

//#include "hdr/typedefs.h"

// included only for testing
#include "imp/array_static.c"
#include "imp/util.c"

int main(int argc, char *argv[]) {
	append_block();
	append_block();
	append_block();

	pop_block();
	pop_block();
	pop_block();
	pop_block();
	pop_block();
	pop_block();
	pop_block();
	
	append_block();
append_block();
append_block();
append_block();
append_block();
append_block();
append_block();
append_block();
append_block();


	printf("%d\n", find_most_matching_element());

	return 0;
}
