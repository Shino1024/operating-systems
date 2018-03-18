#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

int gen_record(record dest, unsigned int size) {
	if (dest == NULL) {
		return -1;
	}

	struct timeval rand_tv;
	gettimeofday(&rand_tv, NULL);
	srand48(rand_tv.tv_usec + (rand_tv.tv_sec % 1000) * 1E6);
	
	unsigned int i;
	for (i = 0; i < size; ++i) {
		dest[i] = (unsigned char) (lrand48() % 94 + 33);
	}

	return 0;
}
/*
int get_nth_record_key(const FILE *file, unsigned int i, unsigned int record_size) {
	if (file == NULL) {
		return -1;
	}

	int error_code;
	error_code = fseek(file, (record_size + 1) * i, 0);
	if (error_code != 0) {
		return -2;
	}

	unsigned char buffer;
	fread(

	fseek

	return 0;
}
*/
int get_nth_record_lib(const FILE *file, unsigned int i, const record record_buffer, unsigned int record_size) {
	if (file == NULL) {
		return -1;
	}

	int error_code;
	error_code = fseek(file, (record_size + 1) * i, 0);
	if (error_code != 0) {
		return -2;
	}

	error_code = fread(record_buffer, sizeof(*record), record_size, file);
	if (error_code != record_size +  1) {
		return -3;
	}

	error_code = fseek(file, 0, 0);
	if (error_code != 0) {
		return -4;
	}

	return 0;
}

int set_nth_record_lib(const FILE *file, unsigned int i, const record record_buffer, unsigned int record_size) {
	if (file == NULL) {
		return -1;
	}

	int error_code;
	error_code = fseek(file, (record_size + 1) * i, 0);
	if (error_code != 0) {
		return -2;
	}

	error_code = fwrite(record_buffer, sizeof(*record), record_size, file);
	if (error_code != record_size) {
		return -3;
	}

	error_code = fseek(file, 0, 0);
	if (error_code != 0) {
		return -4;
	}

	return 0;
}

int get_nth_record_sys(int fd, unsigned int i, unsigned int record_size) {
	return 9;
}

int set_nth_record_sys(int fd, unsigned int i, unsigned int record_size) {
	return 0;
}
/*
int swap_records(const FILE *file, unsigned int i, unsigned int j, record r0, record r1, unsigned int record_size) {
	if (file == NULL) {
		return -1;
	}

	int error_code;

	error_code = fseek(file, 0, 0);
	if (error_code != 0) {
		return -;
	}

	return 0;
}
*/
