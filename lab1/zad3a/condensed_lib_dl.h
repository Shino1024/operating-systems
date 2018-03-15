#ifndef CONDENSED_LIB_DL__H
#define CONDENSED_LIB_DL__H

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

typedef int (*make_static_array_t)(unsigned int array_size, unsigned int block_size);
make_static_array_t make_static_array;

typedef int (*zero_out_static_array_t)();
zero_out_static_array_t zero_out_static_array;

typedef int (*append_block_static_t)(unsigned int index, const char *data);
append_block_static_t append_block_static;

typedef int (*append_block_gen_static_t)(unsigned int index);
append_block_gen_static_t append_block_gen_static;

typedef int (*pop_block_static_t)(unsigned int index);
pop_block_static_t pop_block_static;

typedef int (*find_most_matching_block_static_t)(unsigned int index);
find_most_matching_block_static_t find_most_matching_block_static;

// DYNAMIC
typedef array_dynamic * (*make_array_dynamic_t)(unsigned int array_size, unsigned int block_size);
make_array_dynamic_t make_array_dynamic;

typedef int (*free_array_dynamic_t)(array_dynamic **ad);
free_array_dynamic_t free_array_dynamic;

typedef int (*append_block_dynamic_t)(array_dynamic *ad, unsigned int index, const char *data);
append_block_dynamic_t append_block_dynamic;

typedef int (*append_block_gen_dynamic_t)(array_dynamic *ad, unsigned int index);
append_block_gen_dynamic_t append_block_gen_dynamic;

typedef int (*pop_block_dynamic_t)(array_dynamic *ad, unsigned int index);
pop_block_dynamic_t pop_block_dynamic;

typedef int (*find_most_matching_block_dynamic_t)(array_dynamic *ad, unsigned int index);
find_most_matching_block_dynamic_t find_most_matching_block_dynamic;

#endif // CONDENSED_LIB_DL__H