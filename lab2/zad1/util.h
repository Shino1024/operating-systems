#ifndef UTIL__H
#define UTIL__H

typedef unsigned char * record;

typedef unsigned char ** dataset;

int gen_record(record, unsigned int);

int insertion_sort(dataset data, unsigned int data_size, unsigned int record_size);

#endif // UTIL__H
