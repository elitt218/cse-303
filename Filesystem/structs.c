#include "structs.h"
#include "string.h"


void *get_nth_block(char* map, int blockno) {
	return map + SECTSIZE * blockno;
}

int check_block_is_used(char* map, int blockno) {
	Bitmap *b;
	if (blockno < NSECTS / 2) {
		b = get_nth_block(map, 1);
	} else {
		b = get_nth_block(map, 2);
		blockno -= (NSECTS / 2);
	}

	int index = blockno / 8;
	int offset = blockno % 8;

	return (b->bitmap[index] & (1 << offset)) != 0;
}

void use_block(char *map, int blockno) {
	Bitmap *b;
	if (blockno < NSECTS / 2) {
		b = get_nth_block(map, 1);
	} else {
		b = get_nth_block(map, 2);
		blockno -= (NSECTS / 2);
	}

	int index = blockno / 8;
	int offset = blockno % 8;

	b->bitmap[index] = b->bitmap[index] | (1 << offset);
}

void free_block(char *map, int blockno) {
	Bitmap *b;
	if (blockno < NSECTS / 2) {
		b = get_nth_block(map, 1);
	} else {
		b = get_nth_block(map, 2);
		blockno -= (NSECTS / 2);
	}

	int index = blockno / 8;
	int offset = blockno % 8;

	b->bitmap[index] = b->bitmap[index] & (~(1 << offset));
}

int get_free_block_and_use(char *map) {
	int i;
	for (i = 0; i < NSECTS; i ++) {
		if (!check_block_is_used(map, i)) {
			use_block(map, i);
			// memset(get_nth_block(map, i), 0xab, SECTSIZE);
			return i;
		}
	}
	return -1;
}

int create_folder(char *map, const char *foldername, int parent_block_no) {
	int blockno = get_free_block_and_use(map);
	File *folder = (File *) get_nth_block(map, blockno);

	strcpy(folder->f_name, foldername);
	folder->f_type = F_DIR;
	folder->f_blocks = 0;

	if (parent_block_no == -1) {
		parent_block_no = blockno;
	}

	add_file_to_folder(map, folder, create_link(map, blockno, "."));
	add_file_to_folder(map, folder, create_link(map, parent_block_no, ".."));
	return blockno;
}

void add_file_to_folder(char *map, File* folder, int file_block_no) {
	folder->direct[folder->f_blocks ++] = file_block_no;
}


int create_link(char *map, int block_to_link_to, const char *linkname) {
	int blockno = get_free_block_and_use(map);
	File *link = (File *) get_nth_block(map, blockno);
	strcpy(link->f_name, linkname);
	link->f_type = F_LINK;
	link->f_blocks = 1;
	link->direct[0] = block_to_link_to;
	return blockno;
}

int create_file(char *map, const char *filename) {
	int blockno = get_free_block_and_use(map);
	File *file = (File *) get_nth_block(map, blockno);

	strcpy(file->f_name, filename);
	file->f_type = F_FILE;
	file->f_blocks = 0;
	file->f_size = 0;
	return blockno;
}


//void rmfile(char *map, File *dir, char *filename) {

//}
/*
void removedirectory(char *map, File *dir, char *dirname) {
	int i;
	for(i = 0; i<dir->f_blocks; i++) {
		File *file = (File *)get_nth_block(map, i);
		if(file->f_name == dirname) {
			for(i = 0; i<file->f_blocks; i++) {
				File *file1 = (File *)get_nth_block(map, i);
				if(file1->f_type == F_FILE)
					//if this is a file, call removefile function
					rmfile(map, file, file1->f_name);
				else if(file1->f_blocks != 0)
					//if this is a directory and not empty,
					//recursively call removedir
					removedirectory(map, file, file1->f_name);
				else
					//if this is a empty directory, remove it
					rmfile(map, file, file1->f_name);
			}
			break;
		}
	}
	free_block(map, dir->direct[i]);
	//modify file->direct[], blocks--
	int j;
	for(j = i+1; j<(dir->f_blocks); j++) {
		dir->direct[j-1] = dir->direct[j];
	}
	dir->f_blocks--;
}
*/
