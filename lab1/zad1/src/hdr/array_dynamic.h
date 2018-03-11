#ifndef ARRAY_DYNAMIC__H
#define ARRAY_DYNAMIC__H

#include "typedefs.h"

array_dynamic make_array_dynamic(unsigned int array_size, unsigned int block_size);

int free_array_dynamic(array_dynamic ad);

int append_block(array_dynamic *ad);

int pop_block(array_dynamic *ad);

#endif // ARRAY_DYNAMIC__H
