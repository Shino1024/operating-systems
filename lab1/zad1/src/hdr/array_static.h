#ifndef ARRAY_STATIC__H
#define ARRAY_STATIC__H

#include <stdlib.h>

#include "typedefs.h"

#ifndef MAX_BLOCK_NUMBER
	#define MAX_BLOCK_NUMBER 128
#endif // MAX_BLOCK_NUMBER

#ifndef MAX_BLOCK_SIZE
	#define MAX_BLOCK_SIZE 1024
#endif // MAX_BLOCK_SIZE

extern char array_static[MAX_BLOCK_NUMBER][MAX_BLOCK_SIZE];
extern int size;

void zero_out_static_array();

int append_block();

int pop_block();

int find_most_matching_element();

#endif // ARRAY_STATIC__H
