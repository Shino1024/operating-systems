#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main() {
	char buf[20] = { 0 };
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
	//sleep(4);
	int numRead = read(0, buf, 4);
	if (numRead > 0) {
		printf("You said: %s", buf);
	} else {
		printf("oh\n");
	}
	printf("\n%d\n", sizeof(void*));
	memset(buf, 0, 20);
	sleep(4);
	numRead = read(0, buf, 4);
	if (numRead > 0) {
		printf("You said: %s", buf);
	}
}
