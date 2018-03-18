#ifndef UTIL__H
#define UTIL__H

typedef unsigned char * record;

typedef unsigned char ** dataset;

int gen_record(record dest, unsigned int size);

int get_nth_record_lib(const FILE *file, unsigned int i, unsigned int record_size);

int set_nth_record_lib(const FILE *file, unsigned int i, unsigned int reocrd_size);

int get_nth_record_sys(int fd, unsigned int i, unsigned int record_size);

int set_nth_record_sys(int fc, unsigned int i, unsigned int record_size);
/*
int swap_records(const FILE *file, unsigned int i, unsigned int j, record r0, record r1, unsigned int record_size);
*/



#endif // UTIL__H
