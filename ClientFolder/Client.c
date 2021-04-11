/* cli5.c - 	For ICT374 Topic 8
 *              Hong Xie
 *              Last modfied: 16/10/2020
 *		A much improved (still imperfect) version of "cli4.c".
 */



#include <netinet/in.h> /* struct sockaddr_in, htons, htonl */
#include <netdb.h>      /* struct hostent, gethostbyname() */
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include "token.h"
#define BUFSIZE 200000
#define CHUNK_SIZE 512
#define FILE_NAME_SIZE 64
#define SERV_TCP_PORT 40001 /* server's "well-known" port number */

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

void put(int sd, char *fileName)
{

     /*Gets file size*/
     int fileSize = getFileSize(fileName);
     int convertedFileSize = htonl(fileSize);

     /*Gets file Name Length*/
     int fileNameLength = strlen(fileName);
     int convertedFileNameLength = htonl(fileNameLength);

     /*Writes fileNameLength, fileSize and fileName to the server*/
     write(sd, &convertedFileNameLength, sizeof(convertedFileNameLength));
     write(sd, fileName, fileNameLength);

     char opcodeA, opcodeB = 'B';
     char ack;

     read(sd, &opcodeA, sizeof(opcodeA)); //Read Opcode A
     if (opcodeA == 'A')
     {
          read(sd, &ack, sizeof(ack));
          if (ack == '0')
          {
               write(sd, &opcodeB, sizeof(opcodeB));
               write(sd, &convertedFileSize, sizeof(convertedFileSize));

               /*Reads file into buf and writes buf to server*/
               FILE *file;
               int nr = 0;
               char buf[CHUNK_SIZE];
               file = fopen(fileName, "rb");
               while ((nr = fread(buf, 1, CHUNK_SIZE, file)) > 0)
               {
                    if (write(sd, buf, nr) == -1)
                    {
                         printf("Error in sending file\n");
                         break;
                    }
               }
               printf("Successfully sent file: %s\n", fileName);
          }
          else if (ack == '1')
          {
               printf("File name clash\n");
          }
          else if (ack == '2')
          {
               printf("Could not create file\n");
          }
     }
}

void get(int sd, char *fileName)
{
     int nr, nw, i = 0;
     int fileSize = 0, fileNameSize = 0;
     FILE *file;

     //Client writes the filenameLength and then the FileName to the server
     int fileNameLength = strlen(fileName);
     int convertedFileNameLength = htonl(fileNameLength);
     write(sd, &convertedFileNameLength, sizeof(convertedFileNameLength));
     write(sd, fileName, fileNameLength);

     char opcodeA, opcodeB = 'B';
     char ack;

     read(sd, &opcodeA, sizeof(opcodeA)); //Read Opcode A

     if (opcodeA == 'A')
     {
          read(sd, &ack, sizeof(ack));
          if (ack == '0')
          {
               
               /* Reads file size */
               read(sd, &fileSize, sizeof(fileSize));
               fileSize = ntohl(fileSize);

               /* Reads file data */
               int tempChunkSize = CHUNK_SIZE;
               if (CHUNK_SIZE > fileSize)
                    tempChunkSize = fileSize;

               char buf[tempChunkSize];
               int remainingBytes = fileSize;
               file = fopen(fileName, "wb");
               /*While there are still bytes to write*/
               while (remainingBytes > 0)
               {
                    if (tempChunkSize > remainingBytes) //if the chunk size > remaining bytes, reduce the chunk size to remaining bytes
                         tempChunkSize = remainingBytes;

                    nr = read(sd, buf, tempChunkSize); //read current chunk size from socket

                    if (nr == -1) //if nothing more is read, break
                         break;

                    nw = fwrite(buf, 1, nr, file); //write the chunk to the file

                    if (nw < nr)
                         break;

                    remainingBytes -= nw; //subtract bytes written from remaining bytes
               }
               printf("Received file: %s\n", fileName);
               fclose(file);
          }
          else if (ack == '1')
          {
               printf("File does not exist in server\n");
          }
     }
}

int main(int argc, char *argv[])
{
     int sd, n, nr, nw, i = 0, fileSize;
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
     char putOpcode = 'P', quitOpcode = 'Q', getOpcode = 'G';
     char userInput[32];

     while (1)
     {

          /* Gets Client Input*/

          printf("Client Input: ");
          fgets(userInput, sizeof(userInput), stdin);
          nr = strlen(userInput);
          tokenise(userInput, tokens);

          if (strcmp(tokens[0], "put") == 0)
          {
               write(sd, &putOpcode, sizeof(char));
               put(sd, tokens[1]);
          }
          else if (strcmp(tokens[0], "get") == 0)
          {
               write(sd, &getOpcode, sizeof(getOpcode));
               get(sd, tokens[1]);
          }
          else if (strcmp(tokens[0], "exit") == 0)
          {
               write(sd, &quitOpcode, sizeof(quitOpcode));
               exit(0);
          }
     }
}
