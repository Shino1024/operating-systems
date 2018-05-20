#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/times.h>
#include <string.h>

#define MAX_THREADS 32

#define TIMES "Times.txt"

static int error_code;

struct  {
	unsigned int threads_num;
	char *in_file;
	char *filter_file;
	char *out_file;
} program_arguments;

typedef struct image {
	unsigned int height;
	unsigned int width;
	unsigned int max_value;
	unsigned int **data;
} image;

typedef struct filter {
	unsigned int size;
	float **data;
} filter;

typedef struct transform_arguments {
	image *in;
	image *out;
	filter *f;
	unsigned int segment;
} transform_arguments;

int parse_uint(char *source, unsigned int *uint) {
	char *dump;
	*uint = strtoul(source, &dump, 10);
	if (*uint == 0 || *uint >= MAX_THREADS) {
		return -1;
	}

	return 0;
}

int parse_filename(char *source, char *filename) {
	size_t source_len = strlen(source);

	unsigned int i;
	for (i = 0; i < source_len; ++i) {
		if (source[i] == '/') {
			return -1;
		}
	}

	filename = strdup(source);
	if (filename == NULL) {
		return -2;
	}

	return 0;
}

int parse_arguments(int argc, char *argv[]) {
	error_code = parse_uint(argv[1], &(program_arguments.threads_num));
	if (error_code < 0) {
		fprintf(stderr, "Make sure the threads_num is a positive integer smaller than %d.\n", MAX_THREADS + 1);
		return -1;
	}

	error_code = parse_filename(argv[2], program_arguments.in_file);
	if (error_code < 0) {
		fprintf(stderr, "Make sure that in_file doesn't contain '/' characters.");
		return -1;
	}

	error_code = parse_filename(argv[3], program_arguments.filter_file);
	if (error_code < 0) {
		fprintf(stderr, "Make sure that filter__file doesn't contain '/' characters.");
		return -1;
	}

	error_code = parse_filename(argv[4], program_arguments.out_file);
	if (error_code < 0) {
		fprintf(stderr, "Make sure that out_file doesn't contain '/' characters.");
		return -1;
	}

	return 0;
}

int read_in_file(image *in) {
	FILE *in_file = fopen(program_arguments.in_file, "r");
	if (in_file == NULL) {
		fprintf(stderr, "Couldn't open %s for reading.\n", program_arguments.in_file);
		return -1;
	}

	char *buffer = (char *) calloc(sizeof(char), 1 << 16);
	if (buffer == NULL) {
		return -2;
	}

	fscanf(in_file, "%s ", buffer);
	memset(buffer, '\0', 1 << 16);

	fscanf(in_file, "%d %d ", &(in->width), &(in->height));
	fscanf(in_file, "%d ", &(in->max_value));
	in->data = (unsigned int **) calloc(sizeof(unsigned int *), in->height);
	if (in->data == NULL) {
		fclose(in_file);
		return -2;
	}

	unsigned int i;
	for (i = 0; i < in->height; ++i) {
		in->data[i] = (unsigned int *) calloc(sizeof(unsigned int), in->width);
		if (in->data[i] == NULL) {
			fclose(in_file);
			return -2;
		}
	}

	unsigned int h, w;
	for (h = 0; h < in->height; ++h) {
		for (w = 0; w < in->width; ++w) {
			fscanf(in_file, "%d ", &(in->data[h][w]));
		}
	}

	fclose(in_file);

	return 0;
}

int read_filter_file(filter *in) {
	FILE *filter_file = fopen(program_arguments.filter_file, "r");
	if (filter_file == NULL) {
		fprintf(stderr, "Couldn't open %s for reading.\n", program_arguments.filter_file);
		return -1;
	}

	fscanf(filter_file, "%d", &(in->size));
	unsigned int i, j;
	for (i = 0; i < in->size; ++i) {
		for (j = 0; j < in->size; ++j) {
			fscanf(filter_file, "%f ", &(in->data[i][j]));
		}
	}

	fclose(filter_file);

	return 0;
}

int max(int a, int b) {
	if (a > b) {
		return a;
	}
	return b;
}

int min(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

unsigned int transform_value(unsigned int x, unsigned int y, image *in, filter *f) {
	float sum = 0.0f;
	unsigned int i, j;
	unsigned int x1, y1;
	for (i = 0; i < f->size; ++i) {
		for (j = 0; j < f->size; ++j) {
			x1 = min(max(0, x - (int) (f->size / 2 + 0.5) + j), in->width - 1);
			y1 = min(max(0, y - (int) (f->size / 2 + 0.5) + i), in->height - 1);
			sum += in->data[y1][x1] * f->data[y][x];
		}
	}

	return (unsigned int) (sum + 0.5f);
}

void * transform_segment(void *arguments) {
	if (arguments == NULL) {
		return NULL;
	}

	transform_arguments ta = *((transform_arguments *) arguments);
	unsigned int begin = ta.segment * ta.in->width / program_arguments.threads_num;
	unsigned int end = (ta.segment + 1) * ta.in->width / program_arguments.threads_num;

	unsigned int i, j;
	for (i = begin; i < end; ++i) {
		for (j = 0; j < ta.in->height; ++j) {
			ta.out->data[j][i] = transform_value(i, j, ta.in, ta.f);
		}
	}

	return NULL;
}

int dispatch_threads(image *in, image *out, filter *f, pthread_t **threads) {
	out->height = in->height;
	out->width = in->width;
	out->max_value = in->max_value;
	out->data = (unsigned int **) calloc(sizeof(*(out->data)), in->height);
	if (out->data == NULL) {
		return -1;
	}

	unsigned int i;
	for (i = 0; i < in->height; ++i) {
		out->data[i] = (unsigned int *) calloc(sizeof(unsigned int), in->width);
		if (out->data[i] == NULL) {
			return -1;
		}
	}

	for (i = 0; i < program_arguments.threads_num; ++i) {
		transform_arguments *ta = (transform_arguments *) calloc(sizeof(*ta), 1);
		ta->in = in;
		ta->f = f;
		ta->out = out;
		ta->segment = i;
		pthread_create(threads[i], NULL, transform_segment, (void *) ta);
	}

	return 0;
}

int sync_threads(pthread_t *threads) {
	unsigned int i;
	void *argument;
	for (i = 0; i < program_arguments.threads_num; ++i) {
		pthread_join(threads[i], &argument);
	}

	return 0;
}

int note_time(time_t delta) {
	FILE *times_file = fopen(TIMES, "a+");
	if (times_file == NULL) {
		return -1;
	}

	unsigned int milliseconds = delta % 1000;
	unsigned int seconds = (delta / 1000) % 60;
	unsigned int minutes = (delta / 1000) / 60;
	fprintf(times_file, "Result: %d:%d.%d for %d threads.\n", minutes, seconds, milliseconds, program_arguments.threads_num);

	fclose(times_file);
	return 0;
}

int save_file(image out) {
	FILE *out_file = fopen(program_arguments.out_file, "w");
	if (out_file == NULL) {
		fprintf(stderr, "Couldn't open %s for writing.\n", program_arguments.out_file);
		return -1;
	}

	fprintf(out_file, "P2\n");
	fprintf(out_file, "%d %d\n", out.width, out.height);
	fprintf(out_file, "%d\n", out.max_value);
	unsigned int w, h;
	for (h = 0; h < out.height; ++h) {
		for (w = 0; w < out.width; ++w) {
			fprintf(out_file, "%d\t", out.data[h][w]);
		}

		fprintf(out_file, "\n");
	}

	fclose(out_file);

	return 0;
}

int usage(char *program_name) {
	printf("Usage: %s"
			"\n\tthreads_number"
			"\n\tpicture_filename"
			"\n\tfilter_filename"
			"\n\tout_filename"
			"\n", program_name);
	return 0;
}

void atexit_handler() {
	if (program_arguments.in_file != NULL) {
		free(program_arguments.in_file);
	}

	if (program_arguments.filter_file != NULL) {
		free(program_arguments.filter_file);
	}

	if (program_arguments.out_file != NULL) {
		free(program_arguments.out_file);
	}
}

int main(int argc, char *argv[]) {
	if (argc != 5) {
		usage(argv[0]);
		return 0;
	}

	time_t start_time, end_time;

	error_code = atexit(atexit_handler);
	if (error_code != 0) {
		fprintf(stderr, "Failed to set atexit handler. Exiting...\n");
		return 1;
	}

	error_code = parse_arguments(argc, argv);
	if (error_code < 0) {
		fprintf(stderr, "Errors occured while parsing arguments. Exiting...\n");
		return 2;
	}

	image in, out;
	filter f;
	error_code = read_in_file(&in);
	if (error_code < 0) {
		fprintf(stderr, "Couldn't read from %s. Exiting...\n", program_arguments.in_file);
		return 3;
	}

	error_code = read_filter_file(&f);
	if (error_code < 0) {
		fprintf(stderr, "Couldn't read from %s. Exiting...\n", program_arguments.filter_file);

		return 4;
	}

	time(&start_time);
	pthread_t *threads;
	error_code = dispatch_threads(&in, &out, &f, &threads);
	if (error_code < 0) {
		fprintf(stderr, "Dispatching threads failed. Exiting...\n");
		return 5;
	}

	error_code = sync_threads(threads);
	if (error_code < 0) {
		fprintf(stderr, "Couldn't sync threads. Exiting...\n");
		return 6;
	}
	time(&end_time);

	error_code = note_time(end_time - start_time);
	if (error_code < 0) {
		fprintf(stderr, "Couldn't open %s for appending. Exiting...\n", TIMES);
		return 7;
	}

	error_code = save_file(out);
	if (error_code < 0) {
		fprintf(stderr, "Couldn't sync threads or write to %s. Exiting...\n", program_arguments.out_file);
		return 8;
	}

	return 0;
}
