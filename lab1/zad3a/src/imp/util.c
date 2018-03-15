#include <stdlib.h>
#include <sys/time.h>
#include <limits.h>

#include "../hdr/util.h"
#include "../hdr/typedefs.h"

void gen_data(char *blk, unsigned int block_size) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand((unsigned int) ((tv.tv_sec % 1000) * 1E6 + tv.tv_usec));
	unsigned int i;
	for (i = 0; i < block_size; ++i) {
		blk[i] = rand() % (CHAR_MAX + 1);
	}
}
