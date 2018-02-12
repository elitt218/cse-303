#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "support.h"
#include "Server.h"


void help(char *progname)
{
    printf("Usage: %s [OPTIONS]\n", progname);
    printf("Initiate a network file server\n");
    printf("  -m    enable multithreading mode\n");
    printf("  -l    number of entries in the LRU cache\n");
    printf("  -p    port on which to listen for connections\n");
}

void die(const char *msg1, char *msg2)
{
    fprintf(stderr, "%s, %s\n", msg1, msg2);
    exit(0);
}

/*
 * open_server_socket() - Open a listening socket and return its file
 *                        descriptor, or terminate the program
 */
int open_server_socket(int port)
{
    int                listenfd;    /* the server's listening file descriptor */
    struct sockaddr_in addrs;       /* describes which clients we'll accept */
    int                optval = 1;  /* for configuring the socket */
    
    /* Create a socket descriptor */
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        die("Error creating socket: ", strerror(errno));
    }
    
    /* Eliminates "Address already in use" error from bind. */
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
    {
        die("Error configuring socket: ", strerror(errno));
    }
    
    /* Listenfd will be an endpoint for all requests to the port from any IP
     address */
    bzero((char *) &addrs, sizeof(addrs));
    addrs.sin_family = AF_INET;
    addrs.sin_addr.s_addr = htonl(INADDR_ANY);
    addrs.sin_port = htons((unsigned short)port);
    if(bind(listenfd, (struct sockaddr *)&addrs, sizeof(addrs)) < 0)
    {
        die("Error in bind(): ", strerror(errno));
    }
    
    /* Make it a listening socket ready to accept connection requests */
    if(listen(listenfd, 1024) < 0)  // backlog of 1024
    {
        die("Error in listen(): ", strerror(errno));
    }
    
    return listenfd;
}


/*
 * handle_requests() - given a listening file descriptor, continually wait
 *                     for a request to come in, and when it arrives, pass it
 *                     to service_function.  Note that this is not a
 *                     multi-threaded server.
 */
void handle_requests(int listenfd, void (*service_function)(int, int), int param, bool multithread)
{
    while(1)
    {
        /* block until we get a connection */
        struct sockaddr_in clientaddr;
        memset(&clientaddr, 0, sizeof(sockaddr_in));
        socklen_t clientlen = sizeof(clientaddr);
        int connfd;
        if((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) < 0)
        {
            die("Error in accept(): ", strerror(errno));
        }
        
        /* print some info about the connection */
        struct hostent *hp;
        hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if(hp == NULL)
        {
            fprintf(stderr, "DNS error in gethostbyaddr() %d\n", h_errno);
            exit(0);
        }
        char *haddrp = inet_ntoa(clientaddr.sin_addr);
        printf("server connected to %s (%s)\n", hp->h_name, haddrp);
        
        /* serve requests */
        service_function(connfd, param);
        
        /* clean up, await new connection */
        if(close(connfd) < 0)
        {
            die("Error in close(): ", strerror(errno));
        }
    }
}

void file_server(int connfd, int lru_size)
{
    const int READ_AMT = 4096;
    char buffer[READ_AMT];
    int nread;
    char request_type[5];
    request_type[5] = '\0'; //null terminator
    char* start;
    bool is_checksum = false;
    char checksum[17];
    nread = read(connfd, buffer, sizeof(buffer)); //take in the first line: REQUEST_TYPE FILENAME
    strncpy(request_type, buffer, 4); //reachs "PUT " "GET " or "PUTC"
    
    if(strncmp(request_type, "PUT ", 4) == 0) {
        start = buffer+4;
    }
    else if(strncmp(request_type, "GET ", 4) == 0) {
        start = buffer+4;
    }
    else if(strncmp(request_type, "PUTC", 4) == 0) {
        start = buffer+5;
    }
    else if(strncmp(request_type, "GETC", 4) == 0) {
        start = buffer+5;
    } else {
        fprintf(stderr, "ERROR: invalid first line: %s\n see -h for usage", buffer);
        write(connfd, "ERROR: invalid first line:", 30);
        write(connfd, buffer, sizeof(buffer));
    }
    
    char* end = strstr(buffer, "\n");
    int length = end-start;
    char filename[length+2];
    bzero(filename, length+2);
    strncpy(filename, start, length);
    filename[length+1] = '\0';
    //    fprintf(stderr, "filename: %s\n", filename);
    
    //on serverside, get is simply a single line
    if(strncmp(request_type, "GET ", 4) == 0 || strncmp(request_type,"GETC", 4) == 0)
    {
//        fprintf(stderr, "get called\n");
        get_file(connfd, filename);
        return;
    }

    //time to find out how many bytes in contents
    start = end+1;
    end = strstr(start,"\n");
    length = end-start;
    int bytes;
    char c_size[length+1];
    bzero(c_size, length+1);
    //    fprintf(stderr, "csiz %s\n", c_size);
    
    strncpy(c_size, start, length);
    c_size[length+1] = '\0';
    int content_size = atoi(c_size); //retrieves content size in bytes
    //    fprintf(stderr, "bytes: %d\n", content_size);
    
    if(is_checksum)
    {
        start = end+1;
        end = strstr(start, "\n");
        strncpy(checksum, start, 16);//checksum is MD5 digest aka 16
    }
    
    //at this point. It is a PUT request we either read everything, or we still have things to read
    //nremain is the numbe of bytes we still need to read.
    start = end+1;
    int header_size =  end - buffer;
    
    //  fprintf(stderr, "header size: %d\n", header_size);
    
    int contents_read = READ_AMT - header_size;
    size_t nremain = 0;
    
    char contents[contents_read+1];
    bzero(contents, contents_read+1);
    char* extra_content;
    
    strncpy(contents, start, contents_read); //write whatever left you have written to contents
    //  fprintf(stderr, "content: %s\n", contents);
    
    char message[content_size];
    bzero(message,content_size);
    
    strcat(message, contents);
    message[content_size] = '\0';
    
    //fprintf(stderr, "msg now: %s\n", message);
    //fprintf(stderr, "read: %d size: %d \n", contents_read, content_size);
    
    if((contents_read) < content_size) //indicates that there is still more to read.
    {
        nremain = content_size - contents_read;
        extra_content = (char*) malloc (nremain*sizeof(char)+1);
        read(connfd, extra_content, nremain);
        strcat(message, extra_content);
        //        fprintf(stderr, "msg extra: %s\n", extra_content);
    }
    
    bool ok;
    
    if(is_checksum == true){
        ok = put_cfile(filename, message, c_size, checksum);
        if(ok) {
            write(connfd, "OK\n", 3);
        }
    } else if(is_checksum == false){
        FILE *fp = fopen(filename, "wb+"); //open file
        if(fp == NULL) {
            char errmsg[100];
            sprintf(errmsg, "ERROR (%d): %s\n", errno, strerror(errno));
            write(connfd, errmsg, sizeof(errmsg));
            fclose(fp);
            return;
        }
        int writefile = fileno(fp);
        if(write(writefile, message, strlen(message)) == -1){
            char errmsg[100];
            sprintf(errmsg, "ERROR (%d): %s\n", errno, strerror(errno));
            write(connfd, errmsg, sizeof(errmsg));
            fclose(fp);
        }
        write(connfd, "OK\n", 3);
        fclose(fp);
        fprintf(stderr, "OK\n");
    }
    else{
        fprintf(stderr, "ERROR: file not found");
        char errmsg[100];
        sprintf(errmsg, "ERROR (%d): %s\n", errno, strerror(errno));
        write(connfd, errmsg, sizeof(errmsg));
    }
    //by this point we have parsed: request_type, filename, size of file
}


bool put_cfile(char* filename, char* message, char* c_size, char* checksum)
{
    
}


/*
 * get the file size
 */

size_t file_size(char *get_name) {
    FILE *get_size = fopen(get_name, "rb");
    fseek(get_size, 0, SEEK_END);
    size_t size = ftell(get_size);
    fclose(get_size);
    return size;
}


/*
 * get_file() server
 */
void get_file(int connfd, char *filename) {
    FILE* sendfile = fopen(filename, "rb+");
    if(sendfile == NULL) {
        char errmsg[1000];
        sprintf(errmsg, "ERROR (%d): %s\n", errno, strerror(errno));
        write(connfd, errmsg, sizeof(errmsg));
    }
    else if(sendfile != NULL){
//        fprintf(stderr, "filname: %s\n" , filename);
        size_t messagesize = 10;
        //size of filename
        messagesize += sizeof(char*)*strlen(filename);
        size_t filesize = file_size(filename);
//        fprintf(stderr, "filesize: %zd\n", filesize);
        messagesize += filesize/10;
        //file size
        messagesize += filesize;
        
        char msg[messagesize];
        bzero(msg, messagesize);
        
        //message to client
        strcat(msg, "OK ");
        strcat(msg, filename);
        strcat(msg, "\n");

        //bytes
        char strsize[filesize/10];
        sprintf(strsize, "%d", filesize);
        
        int bytes = atoi(strsize);
//        fprintf(stderr, "bytes: %d\n", bytes);
        
        strcat(msg, strsize);
        strcat(msg, "\n");
        
        
        //contents
        char *content = (char*)malloc(sizeof(char*)*filesize);
       
//        int i = 0;
        fread(content, 1, filesize, sendfile); //will write initial 200
/*        while(i < filesize) {
            fread(content+i, 1, 200, sendfile);
            i+=200;
			}*/
        strcat(msg, content);
        strcat(msg, "\n");
        
//        fprintf(stderr, "msg: %s", msg);
//        fprintf(stderr, "content: %s\n", content);
        write(connfd, msg, strlen(msg));
        fclose(sendfile);
    }
}


/*
 * main() - parse command line, create a socket, handle requests
 */
int main(int argc, char **argv)
{
    /* for getopt */
    long opt;
    int  lru_size = 10;
    int  port     = 9022;
    bool multithread = false;
    
    check_team(argv[0]);
    
    /* parse the command-line options.  They are 'p' for port number,  */
    /* and 'l' for lru cache size, 'm' for multi-threaded.  'h' is also supported. */
    while((opt = getopt(argc, argv, "hml:p:")) != -1)
    {
        switch(opt)
        {
            case 'h': help(argv[0]); break;
            case 'l': lru_size = atoi(argv[0]); break;
            case 'm': multithread = true;	break;
            case 'p': port = atoi(optarg); break;
        }
    }
    
    
    /* open a socket, and start handling requests */
    int fd = open_server_socket(port);
    handle_requests(fd, file_server, lru_size, multithread);
    
    exit(0);
}
