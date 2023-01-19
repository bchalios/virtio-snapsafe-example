#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
	int fd, ret;
	size_t gen_counter;

	fd = open(argv[1], 0, S_IRUSR);
	if (fd < 0) {
		fprintf(stderr, "Could not open file: %s\n", argv[1]);
		exit(1);
	}

	ret = read(fd, &gen_counter, sizeof(gen_counter));
	if (ret < 1) {
		perror("Could not read generation counter");
		exit(1);
	}

	fprintf(stdout, "VM generation counter: %lu\n", gen_counter);

	return 0;
}
