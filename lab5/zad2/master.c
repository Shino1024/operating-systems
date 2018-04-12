#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Call the program with the path to the named pipe.");
		return 1;
	}

	mkfifo(argv[1], 0666);

	int fifo_fd = open(argv[1], O_RDONLY);
	if (fifo_fd < 0) {
		perror("open");
		return 2;
	}

	char buffer[MAX_BUFFER_SIZE];
	while (read(fifo_fd, buffer, MAX_BUFFER_SIZE) > 0) {
		printf("%s\n", buffer);
	}

	return 0;
}
