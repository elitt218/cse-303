#ifndef STRUCTS_H
#define STRUCTS_H


#include <assert.h>
/*
 *
 * Define page/sector structures here as well as utility structures
 * such as directory entries.
 *
 * Sectors/Pages are 512 bytes
 * The filesystem is 4 megabytes in size.
 * You will have 8K pages total. 
 *
 */

#define MAXSIZE  4194304  // 4 MB
#define SECTSIZE 512      // 512 Byte
#define NSECTS   (MAXSIZE / SECTSIZE)  // Total number of sectors = 8192

#define MAXNAMELEN 128
#define NDIRECT    128
#define MAGIC      0x4fda321c

#define F_FILE 1
#define F_DIR  2
#define F_LINK 3


typedef struct File {
	char f_name[MAXNAMELEN];  // 128
	int f_blocks; 			  // 4
	int f_type;				  // 4
	int f_size;				  // 4
	short direct[NDIRECT];    // 256
	// padding :)
	char _padding[SECTSIZE - MAXNAMELEN - 3*sizeof(int) - sizeof(short)*NDIRECT];
} __attribute__((packed)) File;


// typedef struct Indirect {
// 	int pointer[SECTSIZE / sizeof(int)];
// } __attribute__((packed)) Indirect;
// static_assert(sizeof(Indirect) == SECTSIZE, "Incorrect Indirect size");


// IMPORTANT
// 1st ([0]) sector must be super node
// 2nd ([1]) and 3rd ([2]) sectors must be bitmap

typedef struct Super {
	int magic;
	int root_block;
	// padding ;)
	char _padding[SECTSIZE - 2*sizeof(int)];
} __attribute__((packed)) Super;


typedef struct Bitmap {
	unsigned char bitmap[SECTSIZE];
} __attribute__((packed)) Bitmap;


void *get_nth_block(char* map, int blockno);
int check_block_is_used(char* map, int blockno);
void use_block(char* map, int blockno);
void free_block(char* map, int blockno);
int get_free_block_and_use(char *map);
int create_folder(char *map, const char *foldername, int parent_block_no);
void add_file_to_folder(char *map, File* folder, int file_block_no);
int create_link(char *map, int block_to_link_to, const char *linkname);
int create_file(char *map, const char *filename);
void rmfile(char *map, File *dir, char *filename);
//void removedirectory(char *map, File *dir, char *dirname);
#endif
