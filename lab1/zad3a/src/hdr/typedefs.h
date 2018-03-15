#ifndef TYPEDEFS__H
#define TYPEDEFS__H

#ifndef MAX_BLOCK_NUMBER
	#define MAX_BLOCK_NUMBER 128
#endif // MAX_BLOCK_NUMBER

#ifndef MAX_BLOCK_SIZE
	#define MAX_BLOCK_SIZE 1024
#endif // MAX_BLOCK_SIZE

typedef char ** block_array;

typedef char * block;

typedef char chunk;

typedef struct array_dynamic {
	unsigned int array_size;
	unsigned int block_size;
	block_array array;
} array_dynamic;

#endif // TYPEDEFS__H
