#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>       
#include <sys/mman.h>
#include "support.h"
#include "structs.h"
#include "filesystem.h"


/* pointer to the memory-mapped filesystem */
char *map;

File *current_dir;
int current_block_no;

typedef struct Node
{
	int path;
	struct Node *next;
} Node;

Node *head = NULL;

void create_node(int val) {
	Node *new_node = malloc(sizeof(Node));
	new_node->path = val;
	new_node->next = head;
	head = new_node;
}
/*
 * generateData() - Converts source from hex digits to
 * binary data. Returns allocated pointer to data
 * of size amt/2.
 */
char* generateData(char *source, size_t size)
{
	char *retval = (char *)malloc((size >> 1) * sizeof(char));

	for(size_t i=0; i<(size-1); i+=2)
	{
		sscanf(&source[i], "%2hhx", &retval[i>>1]);
	}
	return retval;
}

void init_file_system() {
	Super *super = (Super *) get_nth_block(map, 0);
	super->magic = MAGIC;

	memset(get_nth_block(map, 1), 0, SECTSIZE); // set bitmap to be all 0s
	memset(get_nth_block(map, 2), 0, SECTSIZE); // set bitmap to be all 0s

	use_block(map, 0);
	use_block(map, 1);
	use_block(map, 2);

	int root_node_block = create_folder(map, "", -1);
	super->root_block = root_node_block;
}


void validate_file_system() {
	if (sizeof(Super) != SECTSIZE) {
		printf("Incorrect Super size\n");
		exit(1);
	}
	if (sizeof(Bitmap) != SECTSIZE) {
		printf("Incorrect Bitmap size\n");
		exit(1);
	}
	if (sizeof(File) != SECTSIZE) {
		printf("Incorrect File size\n");
		exit(1);
	}
	Super *super = (Super *) get_nth_block(map, 0);

	if (super->magic != MAGIC) {
		fprintf(stderr, "Invalid magic\n");
		exit(1);
	}

	if (!check_block_is_used(map, 0) ||
		!check_block_is_used(map, 1) ||
		!check_block_is_used(map, 2)) {

		fprintf(stderr, "Block 0, 1, 2 should not be free\n");
		exit(1);
	}

	File *root = (File *) get_nth_block(map, super->root_block);
	if (root->f_type != F_DIR) {
		fprintf(stderr, "Root block missing\n");
		exit(1);
	}

	current_dir = root;
	current_block_no = super->root_block;
}


void ls() {
	int i;
	for (i = 0; i < current_dir->f_blocks; i ++) {
		int blockno = current_dir->direct[i];
		File *file = (File *) get_nth_block(map, blockno);
		if(file->f_type == F_FILE) {
			printf("f %12d %-s\n", file->f_size, file->f_name);
		}
		else {
			printf("d %12d %-s\n", file->f_blocks * SECTSIZE, file->f_name);
		}
	}
}

void pwd_recursive(Node *node) {
	if (node->next != NULL) {
		pwd_recursive(node->next);
	}
	File *file = (File*) get_nth_block(map, node->path);
	printf("/%s", file->f_name);
}

void pwd() {
	if (head == NULL) {
		printf("/\n");
	}
	else {
		pwd_recursive(head);
		printf("\n");
	}
}

void mkdir(const char *dirname) {
	int blockno = create_folder(map, dirname, current_block_no);
	add_file_to_folder(map, current_dir, blockno);
}

void cd(const char *dirname) {
	int i;
	for (i = 0; i < current_dir->f_blocks; i ++) {
		int blockno = current_dir->direct[i];
		File *file = (File *) get_nth_block(map, blockno);

		if (file->f_type == F_DIR && strcmp(dirname, file->f_name) == 0) {
			current_dir = file;
			current_block_no = blockno;
			create_node(blockno);
			return;
		}

		if (file->f_type == F_LINK && strcmp(dirname, file->f_name) == 0) {
			int link_to_blockno = file->direct[0];
			File *link_to_file = (File *) get_nth_block(map, link_to_blockno);
			
			current_dir = link_to_file;
			current_block_no = link_to_blockno;
			if (strcmp(dirname, "..") == 0 && head != NULL) {
				head = head->next;
			}
			return;
		}
	}

	printf("[%s] directory not found\n", dirname);
}

void dump(FILE *fp, int pageno) {
	int nlines = SECTSIZE / 32;
	int base = pageno * SECTSIZE;
	int i;
	for (i = 0 ; i < nlines; i ++) {
		int j;
		for (j = 0; j < 32; j ++) {
			fprintf(fp, "%02x", map[base + 32*i + j] & 0xff);
			if (j == 15) {
				fprintf(fp, "    ");
			} else if (j != 31) {
				fprintf(fp, " ");
			}
		}
		fprintf(fp, "\n");
	}
}

int usage_recursive(File *file, int sum) {
	if (file->f_type == F_FILE) {
		sum += file->f_size;
	}
	if (file->f_type == F_DIR) {
		int i;
		for (i = 0; i < file->f_blocks; i++) {
			File *file2 = (File*) get_nth_block(map, file->direct[i]);
			sum = usage_recursive(file2, sum);
		}
	}
	return sum;
}
void usage() {
	int sum = 0;
	int npageused = 0;
	int i;
	for (i = 0; i < NSECTS; i ++) {
		npageused = npageused + check_block_is_used(map, i);
	}
	Super *super = (Super*) get_nth_block(map, 0);
	int root_num = super->root_block;
	File *file = (File*) get_nth_block(map, root_num); // this is root
													   // directory
	sum = usage_recursive(file, sum);
	printf("#%d bytes used\n", sum);
	printf("#%d bytes on disk\n", npageused * SECTSIZE);
}


void rm(const char *filename) {
//	rmfile(map, current_dir, filename);
	int i;
	for (i = 0; i < current_dir->f_blocks; i++) {
		int blockno = current_dir->direct[i];
		File *file = (File *) get_nth_block(map, blockno);

		if (strcmp(file->f_name, filename) == 0) {
			if (file->f_type == F_DIR) {
				fprintf(stderr, "Cannot use `rm` to remove a directory\n");
				return;
			}
			int j;
			for (j = 0; j < file->f_blocks; j++) {
				free_block(map, file->direct[j]);
			}
			free_block(map, current_dir->direct[i]);

			// shifting direct[] entries to replace the removed one
			for (j = i; j < current_dir->f_blocks-1; j++) {
				current_dir->direct[i] = current_dir->direct[j+1];
				i++;
			}
			current_dir->f_blocks--;
			return;
		}
	}
	//  fprintf(stderr, "\"%s\" does not exist\n", filename);
}
/*
void removedir(char *dirname) {
	removedirectory(map, current_dir, dirname);
}
*/
void mywrite(const char *filename, size_t amt, const char *data) {
	rm(filename);
	int blockno = create_file(map, filename);
	add_file_to_folder(map, current_dir, blockno);
	int num_blocks = ((amt-1)/512) + 1; //number of blocks needed
	File *file = (File *) get_nth_block(map, blockno); // get address of the file
	file->f_blocks = num_blocks;
	file->f_size = amt;
	
	int i;
	for (i = 0; i < num_blocks; i++) {
		int blockno = get_free_block_and_use(map);
		file->direct[i] = blockno;
		if (i != num_blocks - 1) {
			memcpy(get_nth_block(map, blockno), data + SECTSIZE*i, SECTSIZE);
		} else {
			memcpy(get_nth_block(map, blockno), data + SECTSIZE*i, amt - SECTSIZE*i);
		}
	}

}

void print(File *file) {
	int i;
	for(i = 0; i < file->f_blocks; i++) {
		char *data = get_nth_block(map, file->direct[i]);
		int size_to_print = SECTSIZE;
		int j;
		if (i == file->f_blocks-1) {
			size_to_print = file->f_size - SECTSIZE*i;
		}
		for (j = 0; j < size_to_print; j++) {
			printf("%c", data[j]);
		}
	}
	printf("\n");
}

void mycat(const char *filename) {
	int i;
	for(i = 0; i < current_dir->f_blocks; i++) {
		int blockno = current_dir->direct[i];
		File *file = (File *) get_nth_block(map, blockno);

		if (strcmp(file->f_name, filename) == 0) {
			if (file->f_type == F_DIR) {
				fprintf(stderr, "Cannot use `cat` command to a directory\n");
				return;
			}
			print(file);
			return;
		}
	}
	fprintf(stderr, "\"%s\" does not exist\n", filename);
}

void append(const char *filename, size_t amt, const char *data) {
	int num_blocks = ((amt-1)/512) + 1; // number of blocks needed
	int i;
	for (i = 0; i < current_dir->f_blocks; i++) {
		int blockno = current_dir->direct[i];
		File *file = (File *) get_nth_block(map, blockno);
		if (strcmp(file->f_name, filename) == 0) {
			if(file->f_type == F_DIR) {
				fprintf(stderr, "Can't append to directory\n");
				return;
			}
			if(file->f_type == F_FILE) {
				blockno = get_nth_block(map, file->f_size);
				if(num_blocks > amt) {
					
				}
			}
		}
	}
}

/*
 * filesystem() - loads in the filesystem and accepts commands
 */
void filesystem(char *file)
{
	/*
	 * open file, handle errors, create it if necessary.
	 * should end up with map referring to the filesystem.
	 */

	// return -1 if file does not exist. it's ok
	int file_exists = access( file, F_OK ); 
	// open the file (will create an empty if none exists)
	int fd = open(file, O_RDWR | O_CREAT, 0644);
	// expand the file size to at least MAXSIZE
	ftruncate(fd, MAXSIZE);
	// mmap! fun things begin
	map = mmap(NULL, MAXSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (file_exists == -1) {
		init_file_system();
	}

	validate_file_system();

	/* You will probably want other variables here for tracking purposes */


	/*
	 * Accept commands, calling accessory functions unless
	 * user enters "quit"
	 * Commands will be well-formatted.
	 */
	char *buffer = NULL;
	size_t size = 0;
	while(getline(&buffer, &size, stdin) != -1)
	{
		/* Basic checks and newline removal */
		size_t length = strlen(buffer);
		if(length == 0)
		{
			continue;
		}
		if(buffer[length-1] == '\n')
		{
			buffer[length-1] = '\0';
		}

		/* TODO: Complete this function */
		/* You do not have to use the functions as commented (and probably can not)
		 *	They are notes for you on what you ultimately need to do.
		 */

		if(!strcmp(buffer, "quit"))
		{
			break;
		}
		else if(!strncmp(buffer, "dump ", 5))
		{
			if(isdigit(buffer[5]))
			{
				dump(stdout, atoi(buffer + 5));
			}
			else
			{
				char *filename = buffer + 5;
				char *space = strstr(buffer+5, " ");
				*space = '\0';
				//open and validate filename
				FILE *fp = fopen(filename, "w");
				dump(fp, atoi(space + 1));
				fclose(fp);
			}
		}
		else if(!strncmp(buffer, "usage", 5))
		{
			usage();
		}
		else if(!strncmp(buffer, "pwd", 3))
		{
			pwd();
//			printf("\n");
		}
		else if(!strncmp(buffer, "cd ", 3))
		{
			cd(buffer+3);
		}
		else if(!strncmp(buffer, "ls", 2))
		{
			ls();
		}
		else if(!strncmp(buffer, "mkdir ", 6))
		{
			mkdir(buffer+6);
		}
		else if(!strncmp(buffer, "cat ", 4))
		{
			mycat(buffer + 4);
		}
		else if(!strncmp(buffer, "write ", 6))
		{
			char *filename = buffer + 6;
			char *space = strstr(buffer+6, " ");
			*space = '\0';
			size_t amt = atoi(space + 1);
			space = strstr(space+1, " ");

			char *data = generateData(space+1, amt<<1);
			mywrite(filename, amt, data);
			free(data);
		}
		else if(!strncmp(buffer, "append ", 7))
		{
			char *filename = buffer + 7;
			char *space = strstr(buffer+7, " ");
			*space = '\0';
			size_t amt = atoi(space + 1);
			space = strstr(space+1, " ");

			char *data = generateData(space+1, amt<<1);
			append(filename, amt, data);
			free(data);
		}
		else if(!strncmp(buffer, "getpages ", 9))
		{
			//getpages(buffer + 9);
		}
		else if(!strncmp(buffer, "get ", 4))
		{
			char *filename = buffer + 4;
			char *space = strstr(buffer+4, " ");
			*space = '\0';
			size_t start = atoi(space + 1);
			space = strstr(space+1, " ");
			size_t end = atoi(space + 1);
			//get(filename, start, end);
		}
		else if(!strncmp(buffer, "rmdir ", 6))
		{
//			removedir(buffer + 6);
		}
		else if(!strncmp(buffer, "rm -rf ", 7))
		{
			//rmForce(buffer + 7);
		}
		else if(!strncmp(buffer, "rm ", 3))
		{
			rm(buffer + 3);
		}
		else if(!strncmp(buffer, "scandisk", 8))
		{
			//scandisk();
		}
		else if(!strncmp(buffer, "undelete ", 9))
		{
			//undelete(buffer + 9);
		}



		free(buffer);
		buffer = NULL;
	}
	free(buffer);
	buffer = NULL;

}

/*
 * help() - Print a help message.
 */
void help(char *progname)
{
	printf("Usage: %s [FILE]...\n", progname);
	printf("Loads FILE as a filesystem. Creates FILE if it does not exist\n");
	exit(0);
}

/*
 * main() - The main routine parses arguments and dispatches to the
 * task-specific code.
 */
int main(int argc, char **argv)
{
	/* for getopt */
	long opt;

	/* run a student name check */
	check_student(argv[0]);

	/* parse the command-line options. For this program, we only support */
	/* the parameterless 'h' option, for getting help on program usage. */
	while((opt = getopt(argc, argv, "h")) != -1)
	{
		switch(opt)
		{
		case 'h':
			help(argv[0]);
			break;
		}
	}

	if(argv[1] == NULL)
	{
		fprintf(stderr, "No filename provided, try -h for help.\n");
		return 1;
	}

	filesystem(argv[1]);
	return 0;
}
