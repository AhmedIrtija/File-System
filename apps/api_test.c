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

int checkMount(const char *diskname){
	int ret;

	ret = fs_mount("non-existent");
	ASSERT(ret, "fs_mount");

	ret = fs_mount(diskname);

	return ret;
}


int checkUnmount(const char *diskname){
	int ret;

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	ret = fs_umount();
	ASSERT(!ret, "fs_umount");

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	ret = fs_umount();

	return ret;
}

int checkInfo(const char *diskname){
	int ret;

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	ret = fs_info();
	ASSERT(!ret, "fs_info");

	ret = fs_umount();

	ASSERT(!ret, "fs_umount");

	return ret;

}


int checkCreate(const char *diskname){
	int ret;
	char *filename = "test";


	ret = fs_create(filename);
	ASSERT(ret, "fs_create1");

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	ret = fs_create(filename);
	ASSERT(!ret, "fs_create2");

	ret = fs_create(filename);
	ASSERT(ret, "fs_create3");

	ret = fs_umount();
	ASSERT(!ret, "fs_umount");

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	filename = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

	ret = fs_create(filename);
	ASSERT(ret, "fs_create4");

	return 0;
}

int checkDelete(const char *diskname){
	int ret;
	char *filename = "myfile";

	ret = fs_create(filename);
	ASSERT(!ret, "fs_create1");

	ret = fs_delete(filename);
	ASSERT(!ret, "fs_delete1");

	return ret;
}


int checkLs(const char *diskname){
	int ret; 

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	ret = fs_ls();
	ASSERT(!ret, "fs_ls");

	ret =  fs_umount();
	ASSERT(!ret, "fs_umount");

	return ret;
}


int checkOpen(const char *diskname){
	int ret;
	char *filename = "text";

	ret = fs_open(filename);
	ASSERT(ret, "fs_open");

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	ret = fs_open(filename);
	ASSERT(!ret, "fs_open");

	ret = fs_open(filename);
	ASSERT(ret, "fs_open");

	return ret;
}

int checkClose(const char *diskname){
	int ret;
	char *filename = "myfile";
	int fd;

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	fd = fs_open(filename);
	ASSERT(!ret, "fs_open");

	ret = fs_close(fd);
	ASSERT(!ret, "fs_close");

	ret = fs_umount();
	ASSERT(!ret, "fs_umount");

	return ret;
}

int checkStat(const char *diskname){
	int ret;
	int fd;
	char *filename = "test";

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	fd = fs_open(filename);
	ASSERT(fd >= 0, "fs_open");

	ret = fs_stat(fd);
	ASSERT(!ret, "fs_stat");

	ret = fs_close(fd);
	ASSERT(!ret, "fs_close");

	ret = fs_umount();
	ASSERT(!ret, "fs_umount");

	return ret;
}

int checkWrite(const char *diskname){
	int ret;
	int fd;
	char data[26] = "abcdefghijklmnopqrstuvwxyz";
	char *filename = "myfile.txt";

	ret = fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	ret = fs_create(filename);
	ASSERT(!ret, "fs_create");

	fd = fs_open(filename);
	ASSERT(fd >= 0, "fs_open");

	ret = fs_write(fd, data, sizeof(data));
	ASSERT(ret == sizeof(data), "fs_write");

	fs_close(fd);
	fs_umount();

	return 0;
}

int checkRead(const char *diskname){
	int ret;
	int fd;
	char data[26];
	char *filename = "myfile.txt";

	ret =  fs_mount(diskname);
	ASSERT(!ret, "fs_mount");

	fd = fs_open(filename);
	ASSERT(fd >= 0, "fs_open");

	fs_lseek(fd, 12);

	ret = fs_read(fd, data, 10);
	ASSERT(ret == 10, "fs_read");

	ASSERT(!strncmp(data, "mnopqrstuv", 10), "fs_read");


	fs_close(fd);
	fs_umount();

	return 0;
}



int main(int argc, char *argv[])
{
	int ret;
	char *diskname;
	// int fd;
	// char data[26];
	if (argc < 2) {
		printf("Usage: %s <diskimage>\n", argv[0]);
		exit(1);
	}
	/* Mount disk */
	diskname = argv[1];
	int check = -1;

	while(check != 0){
		printf("1 - Check mount\n2 - Check unmount\n3 - Check info\n4 - Check create\n5 - Check delete\n6 - Check ls\n7 - Check open\n8 - Check close\n9 - Check stat\n10 - Check write\n11 - Check read\n0 - Exit\n");
		if (scanf("%d", &check) != 1) {
        	// handle error
        	printf("Invalid input\n");
        	return 0; 
    	}

		switch(check){
			case 1:
				ret = checkMount(diskname);
				ASSERT(!ret, "fs_mount");
				ret = fs_umount();
				ASSERT(!ret, "fs_umount");
				printf("fs_mount successful\n");
				break;
			case 2:
				ret = checkUnmount(diskname);
				ASSERT(!ret, "fs_umount");
				printf("fs_umount successful\n");
				break;
			case 3:
				checkInfo(diskname);
				printf("fs_info successful\n");
				break;
			case 4:
				checkCreate(diskname);
				printf("fs_create successful\n");
				break;
			case 5:
				checkDelete(diskname);
				printf("fs_delete successful\n");
				break;
			case 6:
				checkLs(diskname);
				printf("fs_ls successful\n");
				break;
			case 7:
				checkOpen(diskname);
				printf("fs_open successful\n");
				break;
			case 8:
				checkClose(diskname);
				printf("fs_close successful\n");
				break;
			case 9:
				checkStat(diskname);
				printf("fs_stat successful\n");
				break;
			case 10:
				checkWrite(diskname);
				printf("fs_write successful\n");
				break;
			case 11:
				checkRead(diskname);
				printf("fs_read successful\n");
				break;
			case 0:
			printf("Ending program\n");
				break;
		}

	}
	

	return 0;
}

