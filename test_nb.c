#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

char buffer[4096];

int main(int argc, char* argv[]) {
	int delay = 1, n, m;
	if (argc > 1)
		delay = atoi(argv[1]);
	fcntl(0, F_SETFL , fcntl(0,F_GETFL) | O_NONBLOCK);
	fcntl(1, F_SETFL , fcntl(1,F_GETFL) | O_NONBLOCK);

	while (1) {
		n = read(0, buffer, 4096);
		if (n >= 0) {
			m = write(1,buffer,n);
		}
		if ((n < 0 || m < 0) && (errno != EAGAIN)) {
			break;
		}
		sleep(delay);
	}
	perror(n < 0 ? "stdin" : "stdout");
	exit(1);
}
