#ifndef ARRAY_DYNAMIC__H
#define ARRAY_DYNAMIC__H

typedef char ** block_array;

typedef char * block;

typedef char chunk;

typedef struct array_dynamic {
	unsigned int array_size;
	unsigned int block_size;
	block_array array;
} array_dynamic;

array_dynamic make_array_dynamic(unsigned int array_size, unsigned int block_size);

int free_array_dynamic(array_dynamic ad);

#endif // ARRAY_DYNAMIC__H
