#ifndef UTIL__H
#define UTIL__H

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

//#include "typedefs.h"

//#define MAX(type, a, b) ((type) ((type) (a) > (type) (b) ? (type) (a) : (type) (b)))

#define ABS(x) (x) > 0 ? (x) : -(x)

void gen_data(char *blk, unsigned int block_size);

#endif // UTIL__H
