#ifndef ARRAY_DYNAMIC__H
#define ARRAY_DYNAMIC__H

#include "typedefs.h"

array_dynamic * make_array_dynamic(unsigned int array_size, unsigned int block_size);

int free_array_dynamic(array_dynamic **ad);

int append_block_dynamic(array_dynamic *ad, unsigned int index);

int pop_block_dynamic(array_dynamic *ad, unsigned int index);

int find_most_matching_element_dynamic(array_dynamic *ad);

#endif // ARRAY_DYNAMIC__H
