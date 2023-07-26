#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/*Names of already made Constants and their value
FS_FILENAME_LEN 16

FS_FILE_MAX_COUNT 128

FS_OPEN_MAX_COUNT 32

BLOCK_SIZE 4096

*/
//Constants 
#define Half 2048
#define FAT_EOC 65535

// first block of the file system
typedef struct SUPERBLOCK 
{
	char signature[8];
	uint16_t virBlkAmt;
	uint16_t rootIndex;
	uint16_t dataIndex;
	uint16_t dataBlkAmt;
	uint8_t fatBlkAmt;
	uint8_t padding[BLOCK_SIZE - 17];
} sb;

// root directory stores 128 entries
// ENTRY
typedef struct ROOT 
{
	char filename[FS_FILENAME_LEN];
	uint32_t file_size;
	uint16_t index_first;
	uint8_t padding[10]; // size of entry (32) - 10
} rd;


typedef struct FD_TABLE 
{
	int table_offset;
	int loc; 
} fd;

//Initializing variables
sb superblock;
//FAT can be any size so we just set to pointer for now
uint16_t *FAT_array;

rd rootDir[FS_FILE_MAX_COUNT];
fd FD_table[FS_OPEN_MAX_COUNT];

//Checking list
int mounted;
int rootFreeCount = FS_FILE_MAX_COUNT;
int fatFreeCount;
int fdFreeCount = FS_OPEN_MAX_COUNT;


/**
 * fs_mount - Mount a file system
 * @diskname: Name of the virtual disk file
 *
 * Open the virtual disk file @diskname and mount the file system that it
 * contains. A file system needs to be mounted before files can be read from it
 * with fs_read() or written to it with fs_write().
 *
 * Return: -1 if virtual disk file @diskname cannot be opened, or if no valid
 * file system can be located. 0 otherwise.
 */
int fs_mount(const char *diskname)
{
	if(!strlen(diskname)){
		fprintf(stderr, "Error: Empty Disk name\n");
		return -1;
	}

	if(block_disk_open(diskname) == -1){
		return -1;
	}


	//read the superblock and check if its correct
	if(block_read(0, &superblock) == -1){
		return -1;
	}

	if(memcmp(superblock.signature, "ECS150FS", 8) != 0){
		return -1;
	}

	if(superblock.virBlkAmt != block_disk_count()){
		return -1;
	}


	// initialize root directory by reading it
	if(block_read(superblock.rootIndex, &rootDir) == -1){
		return -1;
	}

	for(size_t i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(rootDir[i].filename[0] != '\0'){
			rootFreeCount--;
		}
	}
	


	FAT_array = (uint16_t*)malloc(sizeof(uint16_t) * superblock.dataBlkAmt);
	fatFreeCount = superblock.dataBlkAmt;
	uint16_t buf[Half];
	int offset = 0;
	// read to the FAT
	for (int i = 1; i <= superblock.fatBlkAmt; i++) {
		if(block_read(i, &buf) == -1){
			return -1;
		}

		for(int j = 0; j < Half; j++){
			if(offset == superblock.dataBlkAmt){
				break;
			}

			FAT_array[offset] = buf[j];
			if(FAT_array[offset] != 0){
				fatFreeCount--;
			}

			offset++;
		}

		if(offset == superblock.dataBlkAmt){
			break;
		}
	}

	
	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++){
		FD_table[i].table_offset = -1;
		FD_table[i].loc = -1;
	}

	mounted = 1;
	return 0;
}


/**
 * fs_umount - Unmount file system
 *
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 *
 * Return: -1 if no FS is currently mounted, or if the virtual disk cannot be
 * closed, or if there are still open file descriptors. 0 otherwise.
 */
int fs_umount(void)
{
	if(!mounted || block_disk_close() == -1){
		return -1;
	}

	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++){
		if(FD_table[i].loc != -1){
			return -1;
		}
	}
	

	free(FAT_array);
	fatFreeCount = 0;

	
	memset(&rootDir, 0, sizeof(rd));

	memset(&superblock, 0, sizeof(sb));

	mounted = 0;

	return 0;
}


/**
 * fs_info - Display information about file system
 *
 * Display some information about the currently mounted file system.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_info(void)
{
	if(!mounted){
		return -1;
	}

	printf("FS Info:\n");
    printf("total_blk_count=%d\n",superblock.virBlkAmt);
    printf("fat_blk_count=%d\n",superblock.fatBlkAmt);
    printf("rdir_blk=%d\n",superblock.rootIndex);
    printf("data_blk=%d\n",superblock.dataIndex);
    printf("data_blk_count=%d\n",superblock.dataBlkAmt);
    printf("fat_free_ratio=%d/%d\n", fatFreeCount, superblock.dataBlkAmt);
    printf("rdir_free_ratio=%d/%d\n", rootFreeCount, FS_FILE_MAX_COUNT);

    return 0;
}


/**
 * fs_create - Create a new file
 * @filename: File name
 *
 * Create a new and empty file named @filename in the root directory of the
 * mounted file system. String @filename must be NULL-terminated and its total
 * length cannot exceed %FS_FILENAME_LEN characters (including the NULL
 * character).
 *
 * Return: -1 if no FS is currently mounted, or if @filename is invalid, or if a
 * file named @filename already exists, or if string @filename is too long, or
 * if the root directory already contains %FS_FILE_MAX_COUNT files. 0 otherwise.
 */
int fs_create(const char *filename)
{
	if(!mounted || filename[0] == '\0' || strlen(filename) > FS_FILENAME_LEN 
	|| rootFreeCount == 0){
		return -1;
	}
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(strcmp(rootDir[i].filename, filename) == 0){
			return -1;
		}
	}


	for(int j = 0; j < FS_FILE_MAX_COUNT; j++){
		if(rootDir[j].filename[0] == '\0'){
			strcpy(rootDir[j].filename, filename);

			rootDir[j].file_size = 0;
			rootDir[j].index_first = FAT_EOC;

			if(block_write(superblock.rootIndex, &rootDir) == -1){
				return -1;
			}
			rootFreeCount--;
			return 0;
		}
	}

	
	return -1;

}

void fs_fat_delete(uint16_t loc){
	if (FAT_array[loc] == 0) {
        return;
    }

	if(FAT_array[loc] == FAT_EOC){
		FAT_array[loc] = 0;
		fatFreeCount++;
		return;
	} else{
		uint16_t next_loc = FAT_array[loc];
		FAT_array[loc] = 0;
		fatFreeCount++;
		fs_fat_delete(next_loc);
	}
}
/**
 * fs_delete - Delete a file
 * @filename: File name
 *
 * Delete the file named @filename from the root directory of the mounted file
 * system.
 *
 * Return: -1 if no FS is currently mounted, or if @filename is invalid, or if
 * Return: -1 if @filename is invalid, if there is no file named @filename to
 * delete, or if file @filename is currently open. 0 otherwise.
 */
int fs_delete(const char *filename) {
	if(!mounted || filename[0] == '\0' || strlen(filename) > FS_FILENAME_LEN){
		return -1;
	}
	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++){
		if(FD_table[i].loc != -1){
			return -1;
		}
	}


	// iterate through files
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(strcmp(rootDir[i].filename, filename) == 0){
			fs_fat_delete(rootDir[i].index_first);
			rootDir[i].filename[0] = '\0';
			rootDir[i].file_size = 0;
			rootDir[i].index_first = 0;
			rootFreeCount++;
			
			// write changes to FAT onto disk
			for(int i = 1; i <= superblock.fatBlkAmt; i++){
				block_write(i, &FAT_array[(i - 1) * Half]);
			}

			// write changes to the root onto disk
			if(block_write(superblock.rootIndex, &rootDir) == -1){
				return -1;
			}

			return 0;
		}
	}

	return -1;
}


/**
 * fs_ls - List files on file system
 *
 * List information about the files located in the root directory.
 *
 * Return: -1 if no FS is currently mounted. 0 otherwise.
 */
int fs_ls(void)
{
	if(!mounted){
		return -1;
	}
	
	printf("FS Ls:\n");
	for(size_t i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if(rootDir[i].filename[0] != '\0'){
			printf("file: %s, size: %d, data_blk: %d\n", 
			rootDir[i].filename, rootDir[i].file_size, rootDir[i].index_first);
		}
	}
	return 0;
}


/**
 * fs_open - Open a file
 * @filename: File name
 *
 * Open file named @filename for reading and writing, and return the
 * corresponding file descriptor. The file descriptor is a non-negative integer
 * that is used subsequently to access the contents of the file. The file offset
 * of the file descriptor is set to 0 initially (beginning of the file). If the
 * same file is opened multiple files, fs_open() must return distinct file
 * descriptors. A maximum of %FS_OPEN_MAX_COUNT files can be open
 * simultaneously.
 *
 * Return: -1 if no FS is currently mounted, or if @filename is invalid, or if
 * there is no file named @filename to open, or if there are already
 * %FS_OPEN_MAX_COUNT files currently open. Otherwise, return the file
 * descriptor.
 */
int fs_open(const char *filename)
{
	if(!mounted || filename[0] == '\0' || strlen(filename) > FS_FILENAME_LEN 
	|| fdFreeCount < 1 ){
		return -1;
	}


	int location = 0;
	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++){
		if(strcmp(rootDir[i].filename, filename) == 0){
			for(int j = 0; j < FS_OPEN_MAX_COUNT; j++){
				if(FD_table[j].loc == -1){
					FD_table[j].table_offset = 0;
					FD_table[j].loc = location;
					location = j;
					break;
				}
			}
			fdFreeCount--;
			return location;
		}
	}
	
	return -1;

}


/**
 * fs_close - Close a file
 * @fd: File descriptor
 *
 * Close file descriptor @fd.
 *
 * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
 * invalid (out of bounds or not currently open). 0 otherwise.
 */
int fs_close(int fd)
{
	if(!mounted || fd >= FS_OPEN_MAX_COUNT || fd < 0 || FD_table[fd].loc == -1){
		return -1;
	}

	FD_table[fd].loc = -1;
	FD_table[fd].table_offset = -1;

	fdFreeCount++;

	return 0;
}


/**
 * fs_stat - Get file status
 * @fd: File descriptor
 *
 * Get the current size of the file pointed by file descriptor @fd.
 *
 * Return: -1 if no FS is currently mounted, of if file descriptor @fd is
 * invalid (out of bounds or not currently open). Otherwise return the current
 * size of file.
 */
int fs_stat(int fd)
{
	if(!mounted || fd >= FS_OPEN_MAX_COUNT || fd < 0 || FD_table[fd].loc == -1){
		return -1;
	}

	return (rootDir[FD_table[fd].loc].file_size);
}


/**
 * fs_lseek - Set file offset
 * @fd: File descriptor
 * @offset: File offset
 *
 * Set the file offset (used for read and write operations) associated with file
 * descriptor @fd to the argument @offset. To append to a file, one can call
 * fs_lseek(fd, fs_stat(fd));
 *
 * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
 * invalid (i.e., out of bounds, or not currently open), or if @offset is larger
 * than the current file size. 0 otherwise.
 */
int fs_lseek(int fd, size_t offset)
{
	if(!mounted || fd >= FS_OPEN_MAX_COUNT || fd < 0 || FD_table[fd].loc == -1 
	|| offset > (size_t)fs_stat(fd)){
		return -1;
	}

	FD_table[fd].table_offset = offset;

	return 0;
}


/**
 * fs_write - Write to a file
 * @fd: File descriptor
 * @buf: Data buffer to write in the file
 * @count: Number of bytes of data to be written
 *
 * Attempt to write @count bytes of data from buffer pointer by @buf into the
 * file referenced by file descriptor @fd. It is assumed that @buf holds at
 * least @count bytes.
 *
 * When the function attempts to write past the end of the file, the file is
 * automatically extended to hold the additional bytes. If the underlying disk
 * runs out of space while performing a write operation, fs_write() should write
 * as many bytes as possible. The number of written bytes can therefore be
 * smaller than @count (it can even be 0 if there is no more space on disk).
 *
 * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
 * invalid (out of bounds or not currently open), or if @buf is NULL. Otherwise
 * return the number of bytes actually written.
 */
int fs_write(int fd, void *buf, size_t count)
{
	if (!mounted || fd >= FS_OPEN_MAX_COUNT || fd < 0 || FD_table[fd].loc == -1 
	|| buf == NULL || count == 0) {
    	return -1;
	}

	
	int root = 	FD_table[fd].loc;
	int amount_written = 0;
	int bytes = 0;
	int offset = FD_table[fd].table_offset;
	int first = offset / BLOCK_SIZE;
	int block_offset;
	if (offset % BLOCK_SIZE == 0) {
    	block_offset = 0;
	} else {
    	block_offset = offset - (first * BLOCK_SIZE);
	}

	uint16_t first_data_block = rootDir[root].index_first;
	uint32_t file_size = rootDir[root].file_size;
	uint8_t written[BLOCK_SIZE];


	if(first_data_block == FAT_EOC){
		for(int i = 0; i < superblock.dataBlkAmt; i++){
			if(FAT_array[i]  == 0){
				first_data_block = i;
				rootDir[root].index_first = first_data_block;
				FAT_array[first_data_block] = FAT_EOC;

				if(block_write(superblock.rootIndex, &rootDir) == -1){
					return 0;
				}
			}
		}

	}

	int curr = first_data_block;
	int prev = first_data_block;

	for(int i = 0; i < first; i++){
		prev = curr;
		curr = FAT_array[curr];
	} 


	while(count > 0){
		if(curr == FAT_EOC){
			for(int i = 0; i < superblock.dataBlkAmt; i++){
				if(FAT_array[i]  == 0){
					FAT_array[prev] = i;
					curr = i;
					FAT_array[curr] = FAT_EOC;
					break;
				}
			}

		}

		if(block_read(curr + superblock.dataIndex, &written) == -1){
			break;
		}

		if((int)count < (BLOCK_SIZE - block_offset)){
			bytes = count;
		} else {
			bytes = BLOCK_SIZE - block_offset;
		}


		memcpy(&written[block_offset], buf + amount_written, bytes);

		if(block_write(curr + superblock.dataIndex, written) == -1){
			return 0;
		}

		block_offset = 0;
		amount_written += bytes;
		count -= bytes;
		offset += bytes;
		prev = curr;
		curr = FAT_array[prev];
	}

	if (offset > (int)file_size) {
    	rootDir[root].file_size = offset;
	} else {
    	rootDir[root].file_size = file_size;
	}

	FD_table[fd].table_offset = offset;

	if(block_write(superblock.rootIndex, rootDir) == -1){
		return 0;
	}


	uint16_t buffer[Half];
	for(int i = 1; i <= superblock.fatBlkAmt; i++){
		block_write(i, &FAT_array[(i-1)*Half]);
		block_read(i, buffer);
	}

	return amount_written;
}


/**
 * fs_read - Read from a file
 * @fd: File descriptor
 * @buf: Data buffer to be filled with data
 * @count: Number of bytes of data to be read
 *
 * Attempt to read @count bytes of data from the file referenced by file
 * descriptor @fd into buffer pointer by @buf. It is assumed that @buf is large
 * enough to hold at least @count bytes.
 *
 * The number of bytes read can be smaller than @count if there are less than
 * @count bytes until the end of the file (it can even be 0 if the file offset
 * is at the end of the file). The file offset of the file descriptor is
 * implicitly incremented by the number of bytes that were actually read.
 *
 * Return: -1 if no FS is currently mounted, or if file descriptor @fd is
 * invalid (out of bounds or not currently open), or if @buf is NULL. Otherwise
 * return the number of bytes actually read.
 */
int fs_read(int fd, void *buf, size_t count)
{
	if (!mounted || fd >= FS_OPEN_MAX_COUNT || fd < 0 || FD_table[fd].loc == -1 
	|| buf == NULL || count == 0) {
    	return -1;
	}

	int root = 	FD_table[fd].loc;
	int offset = FD_table[fd].table_offset;
	int first = offset / BLOCK_SIZE;
	int block_offset;
	if (offset % BLOCK_SIZE == 0) {
    	block_offset = 0;
	} else {
    	block_offset = offset - (first * BLOCK_SIZE);
	}
	int amount = (count / BLOCK_SIZE) + (count % BLOCK_SIZE != 0);
	int amount_read = 0;
	int blocks_read = 0;
	int bytes = 0;

	uint16_t first_data_block = rootDir[root].index_first;
	uint32_t file_size = rootDir[root].file_size;
	uint8_t read[BLOCK_SIZE];


	int curr = first_data_block;

	for(int i = 0; i < first; i++){
		curr = FAT_array[curr];
	}

	while(amount_read < amount){
		if(curr == FAT_EOC){
			break;
		}

		if(block_read(curr + superblock.dataIndex, read) == -1){
			break;
		}

		if((int)count < (BLOCK_SIZE - block_offset)){
			bytes = count;
		} else {
			bytes = BLOCK_SIZE - block_offset;
		}

		if (bytes + offset > (int)file_size) {
    		bytes = file_size - offset;
		}

		memcpy(buf + amount_read, &read[block_offset], bytes);

		amount_read += bytes;
		count -= bytes;
		offset += bytes;
		block_offset = 0;
		blocks_read++;
		curr = FAT_array[curr];
	}

	FD_table[fd].table_offset += offset;

	return amount_read;
}
