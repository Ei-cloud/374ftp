/* cli5.c - 	For ICT374 Topic 8
 *              Hong Xie
 *              Last modfied: 16/10/2020
 *		A much improved (still imperfect) version of "cli4.c".
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* struct sockaddr_in, htons, htonl */
#include <netdb.h>      /* struct hostent, gethostbyname() */
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "token.h"
#define BUFSIZE 200000
#define FILE_BLOCK_SIZE 512
#define FILE_NAME_SIZE 64
#define SERV_TCP_PORT 40001 /* server's "well-known" port number */

typedef struct Payload
{
     char *data;
     char *fileName;
     int fileSize;
     char opcode;

} Payload;

int getFileSize(char *fileName)
{
     FILE *file;
     int fileLen = -1;

     file = fopen(fileName, "rb");
     if (!file)
     {
          perror("FAIL: ");
          system("pwd");
          fprintf(stderr, "Unable to open file %s", fileName);
          return 1;
     }

     fseek(file, 0, SEEK_END);
     fileLen = ftell(file);
     fseek(file, 0, SEEK_SET);
     fclose(file);
     return fileLen;
}

int write_nbytes(int sd, char *buf, int nbytes)
{
     int nw = 0;
     int n = 0;
     int i = 0;
     for (n = 0; n < nbytes; n += nw)
     {
          
          if ((nw = write(sd, buf + n, nbytes - n)) <= 0)

               return (nw); /* write error */
     }
     return n;
}

void put(int sd, char *fileName){

     /*Gets file size*/
     int fileSize = getFileSize(fileName);
     printf("FILE SIZE: %d\n", fileSize);
     int convertedFileSize = htonl(fileSize);
     printf("CONVERTED FILE SIZE IN CLIENT IS: %d\n", convertedFileSize);

     /*Gets file Name Length*/
     int fileNameLength = strlen(fileName);
     int convertedFileNameLength = htonl(fileNameLength);

     /*Writes fileNameLength, fileSize and fileName to the server*/
     write (sd, &convertedFileNameLength, sizeof(convertedFileNameLength));
     write(sd, &convertedFileSize, sizeof(convertedFileSize));
     write(sd, fileName, fileNameLength);

     /*Reads file into buf and writes buf to server*/
     FILE *file;
     char buf[BUFSIZE];
     file = fopen(fileName, "rb");
     fread(buf, fileSize, sizeof(unsigned char), file);
     fclose(file);
     write(sd, buf, fileSize);
}

int main(int argc, char *argv[])
{
     int sd, n, nr, nw, i = 0, fileSize;
     Payload pay;
     char host[BUFSIZE], fileName[FILE_NAME_SIZE];
     //unsigned char *buf;
     struct sockaddr_in ser_addr;
     struct hostent *hp;

     /* get server host name */
     if (argc == 1) /* assume server running on the local host */
          strcpy(host, "localhost");
     else if (argc == 2) /* use the given host name */
          strcpy(host, argv[1]);
     else
     {
          printf("Usage: %s [<server_host_name>]\n", argv[0]);
          exit(1);
     }

     /* get host address, & build a server socket address */
     bzero((char *)&ser_addr, sizeof(ser_addr));
     ser_addr.sin_family = AF_INET;
     ser_addr.sin_port = htons(SERV_TCP_PORT);
     if ((hp = gethostbyname(host)) == NULL)
     {
          printf("host %s not found\n", host);
          exit(1);
     }

     ser_addr.sin_addr.s_addr = *(u_long *)hp->h_addr;

     /* create TCP socket & connect socket to server address */
     sd = socket(PF_INET, SOCK_STREAM, 0);
     if (connect(sd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
     {
          perror("client connect");
          exit(1);
     }

     /* Declaring variables and opcodes*/
     char *tokens[3];
     char putOpcode = 'P', quitOpcode = 'Q';
     char userInput[32];


     while(1){

     /* Gets Client Input*/
     
     printf("Client Input: ");
     fgets(userInput, sizeof(userInput), stdin);
     nr = strlen(userInput);
     tokenise(userInput, tokens);
     printf("TOKENS[0]: %s\n", tokens[0]);
     printf("TOKENS[1]: %s\n", tokens[1]);
     
     printf("FILE NAME: %s\n", fileName);
     if(strcmp(tokens[0], "put") == 0){
          write(sd, &putOpcode, sizeof(char));
          put(sd, tokens[1]);
     }else if(strcmp(tokens[0], "exit") == 0)
     {    
          write(sd, &quitOpcode, sizeof(quitOpcode));
          exit(0);
     }

      printf("sent file: %s\n", tokens[1]);
     }
}

