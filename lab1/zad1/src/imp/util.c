#include "../hdr/util.h"

void gen_data(char *blk, unsigned int block_size) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand((unsigned int) (tv.tv_sec * 1E6 + tv.tv_usec));
	unsigned int i;
	for (i = 0; i < block_size; ++i) {
		blk[i] = rand() % (UCHAR_MAX + 1) + CHAR_MIN;
	}
}
