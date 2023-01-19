#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <assert.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int fd, ret;
	unsigned long gen_counter;
	struct pollfd ufd;

	fd = open(argv[1], 0, S_IRUSR);
	if (fd < 0) {
		perror("Could not open file");
		exit(1);
	}

	ret = read(fd, &gen_counter, sizeof(gen_counter));
	if (ret < 0) {
		perror("read");
		exit(1);
	}

	ufd.fd = fd;
	ufd.events = POLLPRI|POLLERR;

	while (1) {
		ret = poll(&ufd, 1, -1);
		if (ret < 1) {
			perror("poll");
			exit(1);
		}

		/* Can't have 0, we don't have a timeout */
		assert(ret);

		unsigned long new_gen_counter;
		ret = read(fd, &new_gen_counter, sizeof(new_gen_counter));
		if (ret < 0) {
			perror("read");
			exit(1);
		}

		printf("VM generation counter changed. Old: %lu New: %lu\n", gen_counter, new_gen_counter);
		gen_counter = new_gen_counter;
	}

	return 0;
}
