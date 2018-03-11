#ifndef TYPEDEFS__H
#define TYPEDEFS__H

#ifndef MAX_BLOCK_NUMBER
	#define MAX_BLOCK_NUMBER 128
#endif // MAX_BLOCK_NUMBER

#ifndef MAX_BLOCK_SIZE
	#define MAX_BLOCK_SIZE 1024
#endif

typedef struct array_static {
	int size;
	char array[MAX_BLOCK_NUMBER][MAX_BLOCK_SIZE];
} array_static;

typedef char * block;

typedef char chunk;

#endif // TYPEDEFS__H
