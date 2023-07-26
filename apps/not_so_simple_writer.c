#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs.h>

#define ASSERT(cond, func)                               \
do {                                                     \
	if (!(cond)) {                                       \
		fprintf(stderr, "Function '%s' failed\n", func); \
		exit(EXIT_FAILURE);                              \
	}                                                    \
} while (0)

#define DATA_SIZE 4096

int main(int argc, char *argv[])
{
	int ret;
	char *diskname;
	int fd;
	char data[DATA_SIZE];

	if (argc <= 1) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}

	/* Mount disk */
	diskname = argv[1];
	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");
	
	/* Create file and open */
	ret = fs_create("myfile");
	ASSERT(!ret, "fs_create");
	
	fd = fs_open("myfile");
	ASSERT(fd >= 0, "fs_open");

	/* Generate data */
	for (int i = 0; i < DATA_SIZE; i++) {
		data[i] = 'a' + (i % 26);
	}

	/* Write data */
	ret = fs_write(fd, data, sizeof(data));
	ASSERT(ret == sizeof(data), "fs_write");

	/* Close file and unmount */
	ret = fs_close(fd);
	ASSERT(!ret, "fs_close");
	ret = fs_umount();
	ASSERT(!ret, "fs_umount");

	return 0;
}
