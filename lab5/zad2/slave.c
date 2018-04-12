#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define COMMAND "date"

#define MAX_BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s named_pipe number_of_writes\n", argv[0]);
		return 1;
	}

	srand(time(NULL));

	printf("PID: %d\n", getpid());

	int fifo_fd = open(argv[1], O_WRONLY);
	if (fifo_fd < 0) {
		perror("open");
		return 2;
	}

	unsigned int N = atoi(argv[2]);
	FILE *date_file;

	char buffer[MAX_BUFFER_SIZE];

	unsigned int i;
	for (i = 0; i < N; ++i) {
		printf("%dth iter\n", i);
		date_file = popen(COMMAND, "r");
		if (date_file == NULL) {
			perror("popen");
			return 3;
		}

		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%-6d / ", getpid());

		if (fread(buffer + 9, 1, MAX_BUFFER_SIZE - 9, date_file) < 1) {
			perror("fread");
			fclose(date_file);
			return 4;
		}

		if (write(fifo_fd, buffer, MAX_BUFFER_SIZE) < 1) {
			perror("write");
			fclose(date_file);
			return 5;
		}

		fclose(date_file);
		sleep(rand() % 3 + 2);
	}

	return 0;
}
