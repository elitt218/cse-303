#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
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
#include "Client.h"

void help(char *progname)
{
    printf("Usage: %s [OPTIONS]\n", progname);
    printf("Perform a PUT or a GET from a network file server\n");
    printf("  -P    PUT file indicated by parameter\n");
    printf("  -G    GET file indicated by parameter\n");
    printf("  -s    server info (IP or hostname)\n");
    printf("  -p    port on which to contact server\n");
    printf("  -S    for GETs, name to use when saving file locally\n");
    printf("	-c 		use checksum when transferring files");
}

void die(const char *msg1, const char *msg2)
{
    fprintf(stderr, "%s, %s\n", msg1, msg2);
    exit(0);
}

/*
 *getCheckSum() - obtains the checksum of a given the prechecksum message.
 *
 */
char* getCheckSum(char* message){
    unsigned char* digest = (unsigned char*) malloc(MD5_DIGEST_LENGTH);
    MD5_CTX ctx; //struct for md5
    MD5_Init(&ctx);
    MD5_Update(&ctx, message, strlen(message));
    MD5_Final(digest, &ctx);
    
    return (char*)digest;
}
/*
 * connect_to_server() - open a connection to the server specified by the
 *                       parameters
 */
int connect_to_server(char *server, int port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;
    char errbuf[256];                                   /* for errors */
    
    /* create a socket */
    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        die("Error creating socket: ", strerror(errno));
    }
    
    /* Fill in the server's IP address and port */
    if((hp = gethostbyname(server)) == NULL)
    {
        sprintf(errbuf, "%d", h_errno);
        die("DNS error: DNS error ", errbuf);
    }
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);
    
    /* connect */
    if(connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
        die("Error connecting: ", strerror(errno));
    }
    return clientfd;
}
size_t file_size(char *get_name) {
    FILE *get_size = fopen(get_name, "rb+");
    fseek(get_size, 0, SEEK_END);
    size_t size = ftell(get_size);
    fclose(get_size);
    return size;
}

/*
 * put_file() - send a file to the server accessible via the given socket fd
 */
void put_file(int fd, char *put_name)
{
    /* TODO: implement a proper solution, instead of calling the echo() client */
    //	echo_client(fd);
    FILE *ptr = fopen(put_name, "rb+");
    if(ptr == NULL) {
        //      die("File Error ", "File doesn't exist"); //instead ???
        perror("ERROR in opening file");
        exit(1);
    }
    else {
        size_t messagesize = 10;
        //adding size of filename
        messagesize += sizeof(char*)*strlen(put_name);
        size_t filesize = file_size(put_name);
        messagesize += filesize/10;
        //add size of file
        messagesize += filesize;
        
        char msg[messagesize];
        bzero(msg, messagesize);
        
        //add filename to message
        strcat(msg, "PUT ");
        strcat(msg, put_name);
        strcat(msg, "\n");
        
        //and bytes to message
        char strsize[filesize/10];
        sprintf(strsize, "%d", filesize);
        strcat(msg, strsize);
        strcat(msg, "\n");
        
        //add contents to message
        char *content = (char*)malloc(sizeof(char*)*filesize);
//        int i = 0;
        fread(content, 1, filesize, ptr); //will write initial 200
/*        while(i < filesize) {
            fread(content+i, 1, 200, ptr);
            i+=200;
			}*/
        strcat(msg, content);
        strcat(msg, "\n");
        fclose(ptr);
//        fprintf(stderr, "msg: %s\n", msg);
        
        //send the PUT message
        //writes entire file in a single sitting
        write(fd, msg, strlen(msg));
        
        const int MAXLINE = 8192;
        char buf[MAXLINE];
        bzero(buf, MAXLINE);
        read(fd, buf, MAXLINE);
        /*should never happen. only occurs if you read more than MAXLINE and still no \*/
        while(strstr(buf, "\n") == NULL) {
            read(fd, buf, MAXLINE);
        }
        printf("%s", buf);
    }
}
void put_cfile(int fd, char* put_name){
    FILE *fil = fopen(put_name, "rb+");
    if(fil == NULL) {
        //      die("File Error ", "File doesn't exist"); //instead ???
        perror("error in opening");
        exit(EXIT_FAILURE);
    }
    else {
        size_t messagesize = 10;
        //adding size of filename
        messagesize += sizeof(char*)*strlen(put_name);
        size_t filesize = file_size(put_name);
        messagesize += filesize/10;
        //add size of file
        messagesize += filesize;
        
        char* msg = (char*)malloc(messagesize);
        bzero(msg, messagesize);
        
        //add filename to message
        strcat(msg, "PUT ");
        strcat(msg, put_name);
        strcat(msg, "\n");
       
        //and bytes to message
        char strsize[filesize/10];
        sprintf(strsize, "%d", filesize);
        strcat(msg, strsize);
        strcat(msg, "\n");
        
        //add contents to message
        char *content = (char*)malloc(sizeof(char*)*filesize);
        FILE *ptr = fopen(put_name, "rb+");
      
//        int i = 0;
        fread(content, 1, filesize, ptr); //will write initial 200
		/*   while(i < filesize) {
            fread(content+i, 1, 200, ptr);
            i+=200;
			}*/
		
        strcat(msg, content);
        strcat(msg, "\n");
        //fprintf(stderr, "msg: %s\n", msg);
        
        //obtain checksum by giving it the entire messages
        char *checksum = getCheckSum(msg);
        
        char* c_message = (char*) malloc( filesize+strlen(checksum));
        //reassemble the message
        assemble_cput_msg(c_message,put_name, checksum, (filesize+strlen(checksum)), content);
        //printf("cmessage is : %s\n", c_message);
        write(fd, c_message, strlen(c_message));
        free(msg);
        
        
        //to listen for server response
        const int MAXLINE = 8192;
        char buf[MAXLINE];
        bzero(buf, MAXLINE);
        read(fd, buf, MAXLINE);
    }
}

void assemble_cput_msg(char* msg, char* put_name, char* checksum, size_t filesize, char* content)
{
    //add filename to message
    strcat(msg, "PUTC ");
    strcat(msg, put_name);
    strcat(msg, "\n");
    //and bytes to message
    char strsize[filesize/10];
    sprintf(strsize, "%d", filesize);
    strcat(msg, strsize);
    strcat(msg, "\n");
    //add checksum
    strcat(msg, checksum);
    strcat(msg, "\n");
    //add content
    strcat(msg, content);
    strcat(msg, "\n");
    
}


/*
 * get_file() - get a file from the server accessible via the given socket
 *              fd, and save it according to the save_name
 */
void get_file(int fd, char *get_name, char *save_name)
{
    //write to socket a GET request;
    size_t messagesize = 10;
    messagesize += sizeof(char*)*strlen(get_name); //append the length of the filename
    char msg[messagesize];
    bzero(msg,messagesize);
    
    //message
    strcat(msg, "GET ");
    strcat(msg, get_name);
    strcat(msg, "\n");
    write(fd, msg, strlen(msg));
    
    const int READ_AMT = 500;
    char init_buf[READ_AMT+1];
    read(fd, init_buf, READ_AMT);
    
    //name starts after 'ok '
    char* start = init_buf+3;
    //filename at end of newline
    char* end = strstr(init_buf, "\n");
    
    char* okp = strstr(init_buf, "OK ");
    if(okp == NULL) {
        printf("%s\n", init_buf);
        exit(1);
    }
    
    FILE *saveFile = fopen(save_name,"wb+");
    if(saveFile == NULL){
        perror("error in opening");
        fclose(saveFile);
        exit(EXIT_FAILURE);
    }
   
    //get bytes
    start = end+1;
    end = strstr(start, "\n");
    int length = end-start;
    char bytes[length+1];
    //bzero(bytes, length+2);
    
    strncpy(bytes, start, length);
    //		bytes[length+1] = '\0';
    strcat(bytes, "\0");
    int r_bytes = atoi(bytes);
//    fprintf(stderr, "the number of bytes to read in contents is: %d\n", r_bytes);
    
    int headerbytes =(end - init_buf); //header consist of things we have read so far
    start = end+1;
    if(r_bytes < READ_AMT - headerbytes) //bytes to read in contents fits inside of buffer
    {
        //get contents
        char contents[r_bytes+1];
        bzero(contents, r_bytes+1);
        
        strncpy(contents, start, r_bytes);
        contents[r_bytes+1] = '\0';
        //write contents of get_file to save_file
//        fprintf(stderr, "contents: %s\n", contents);
        int writefd = fileno(saveFile);
        write(writefd, contents, strlen(contents));
        fprintf(stderr, "OK\n");
        fclose(saveFile);
    }
    else
    {
        fclose(saveFile);
        saveFile = fopen(save_name,"ab+");
        int writefd = fileno(saveFile);
        int nread = READ_AMT - headerbytes;
        int nremain = r_bytes - nread;
        char contents[nread+1];
        contents[nread+1] = '\0';
        bzero(contents, nread+1);
        strncpy(contents, start, nread);
        write(writefd, contents, nread);
        char contents2[nremain+1];
        read(fd, contents2, nremain);
        write(writefd, contents2, nremain);
        fprintf(stderr, "OK\n" );
        fclose(saveFile);
    }
}



/*
 * main() - parse command line, open a socket, transfer a file
 */
int main(int argc, char **argv)
{
    /* for getopt */
    long  opt;
    char *server = NULL;
    char *put_name = NULL;
    char *get_name = NULL;
    int   port;
    char *save_name = NULL;
    bool checksum = false;
    
    check_team(argv[0]);
    
    /* parse the command-line options. */
    while((opt = getopt(argc, argv, "hs:P:G:S:p:c")) != -1)
    {
        switch(opt)
        {
            case 'h': help(argv[0]); break;
            case 's': server = optarg; break;
            case 'P': put_name = optarg; break;
            case 'G': get_name = optarg; break;
            case 'S': save_name = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'c': checksum = true; break;
        }
    }
    
    /* open a connection to the server */
    int fd = connect_to_server(server, port);
    
    /* put or get, as appropriate */
    if(put_name && !checksum)
    {
        put_file(fd, put_name);
    }
    else if(put_name && checksum){
        printf("%s\n",  "checksum PUT");
        put_cfile(fd, put_name);
    }
    else
    {
        get_file(fd, get_name, save_name);
    }
    
    /* close the socket */
    int rc;
    if((rc = close(fd)) < 0)
    {
        die("Close error: ", strerror(errno));
    }
    exit(0);
}
