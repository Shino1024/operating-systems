#ifndef CONDENSED_LIB__H
#define CONDENSED_LIB__H

// TYPEDEFS
typedef char ** block_array;

typedef char * block;

typedef char chunk;

typedef struct array_dynamic {
	unsigned int array_size;
	unsigned int block_size;
	block_array array;
} array_dynamic;

// UTIL
#define ABS(x) (x) > 0 ? (x) : -(x)

#define ZERO(x) memset((x), 0, sizeof((x)))

void gen_data(char *blk, unsigned int block_size);

// STATIC
#ifndef MAX_BLOCK_NUMBER
	#define MAX_BLOCK_NUMBER 128
#endif // MAX_BLOCK_NUMBER

#ifndef MAX_BLOCK_SIZE
	#define MAX_BLOCK_SIZE 1024
#endif // MAX_BLOCK_SIZE

char array_static[MAX_BLOCK_NUMBER][MAX_BLOCK_SIZE];
char alloc_info[MAX_BLOCK_NUMBER];
unsigned int array_size_static;
unsigned int block_size_static;

int make_static_array(unsigned int array_size, unsigned int block_size);

int zero_out_static_array();

int append_block_static(unsigned int index, const char *data);

int append_block_gen_static(unsigned int index);

int pop_block_static(unsigned int index);

int find_most_matching_block_static(unsigned int index);

// DYNAMIC
array_dynamic * make_array_dynamic(unsigned int array_size, unsigned int block_size);

int free_array_dynamic(array_dynamic **ad);

int append_block_dynamic(array_dynamic *ad, unsigned int index, const char *data);

int append_block_gen_dynamic(array_dynamic *ad, unsigned int index);

int pop_block_dynamic(array_dynamic *ad, unsigned int index);

int find_most_matching_block_dynamic(array_dynamic *ad, unsigned int index);

#endif // CONDENSED_LIB__H