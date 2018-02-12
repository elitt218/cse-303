#pragma once

/*
 * help() - Print a help message
 */
void help(char *progname);

/*
 * die() - print an error and exit the program
 */
void die(const char *msg1, const char *msg2);

/*
 * connect_to_server() - open a connection to the server specified by the
 *                       parameters
 */
int connect_to_server(char *server, int port);

/*
 * echo_client() - this is dummy code to show how to read and write on a
 *                 socket when there can be short counts.  The code
 *                 implements an "echo" client.
 */
void echo_client(int fd);

/*
 * put_file() - send a file to the server accessible via the given socket fd
 */
void put_file(int fd, char *put_name);

/*
 * get_file() - get a file from the server accessible via the given socket
 *              fd, and save it according to the save_name
 */
void get_file(int fd, char *get_name, char *save_name);


/*
 * gets the file size
 */
size_t file_size(char *get_name);

/*
 * putc_file() - put file checksum version
 */
void put_cfile(int fd, char *put_name);

/*
 * checks if file get_name is in directory
 */
//bool is_file(char *get_name);

void assemble_cput_msg(char* msg, char* put_name, char* checksum, size_t filesize, char* content);
