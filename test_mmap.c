#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	int fd, ret;
	void *buf;
	size_t page_size = sysconf(_SC_PAGESIZE);

	fd = open(argv[1], 0, S_IRUSR);
	if (fd < 0) {
		fprintf(stderr, "Could not open file: %s\n", argv[1]);
		exit(1);
	}

	buf = mmap(NULL, page_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!buf) {
		perror("Could not mmap device");
		exit(1);
	}

	size_t gen_counter = *(size_t *)buf;
	while (1) {
		size_t new_gen_counter = *(size_t *)buf;
		if (gen_counter != new_gen_counter) {
			printf("VM generation counter changed! Old: %lu New: %lu\n", gen_counter, new_gen_counter);
		}

		gen_counter = new_gen_counter;
		sleep(10);
	}
}
