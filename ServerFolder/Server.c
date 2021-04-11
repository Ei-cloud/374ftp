

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>     /* strlen(), strcmp() etc */
#include <errno.h>      /* extern int errno, EINTR, perror() */
#include <signal.h>     /* SIGCHLD, sigaction() */
#include <sys/types.h>  /* pid_t, u_long, u_short */
#include <sys/socket.h> /* struct sockaddr, socket(), etc */
#include <sys/wait.h>   /* waitpid(), WNOHAND */
#include <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
                        /* and INADDR_ANY */

#define BUFSIZE 200000
#define SERV_TCP_PORT 40001                 /* server port no */
#define CHUNK_SIZE 512 /* server port no */ /* server port no */

void claim_children()
{
     pid_t pid = 1;

     while (pid > 0)
     { /* claim as many zombies as we can */
          pid = waitpid(0, (int *)0, WNOHANG);
     }
}

void daemon_init(void)
{
     pid_t pid;
     struct sigaction act;

     if ((pid = fork()) < 0)
     {
          perror("fork");
          exit(1);
     }
     else if (pid > 0)
     {
          printf("PID: %d\n", pid);
          exit(0); /* parent terminates */
     }
     /* child continues */
     setsid(); /* become session leader */
     //chdir("/");                    /* change working directory */
     umask(0); /* clear file mode creation mask */

     /* catch SIGCHLD to remove zombies from system */
     act.sa_handler = claim_children; /* use reliable signal */
     sigemptyset(&act.sa_mask);       /* not to block other signals */
     act.sa_flags = SA_NOCLDSTOP;     /* not catch stopped children */
     sigaction(SIGCHLD, (struct sigaction *)&act, (struct sigaction *)0);
     /* note: a less than perfect method is to use 
              signal(SIGCHLD, claim_children);
    */
}

int getFileSize(char *fileName)
{
     FILE *file;
     int fileLen = -1;

     file = fopen(fileName, "rb");
     if (!file)
     {
          perror("FAIL: ");
          fprintf(stderr, "Unable to open file %s", fileName);
          return 1;
     }

     fseek(file, 0, SEEK_END);
     fileLen = ftell(file);
     fseek(file, 0, SEEK_SET);
     fclose(file);
     return fileLen;
}

void serve_put(int sd)
{
     int nr, nw, i = 0;
     char fileName[32];
     bzero(fileName, sizeof(fileName));
     int fileSize = 0, fileNameSize = 0;
     FILE *file;

     /*Reads file name size*/
     nr = read(sd, &fileNameSize, sizeof(fileNameSize));
     fileNameSize = ntohl(fileNameSize);

     /*Reads fileName*/
     nr = read(sd, fileName, fileNameSize);

     /*Defines OPCODES and ACKCODES*/
     char opcodeA = 'A', opcodeB, ackSuccess = '0', ackfileExists = '1', ackCannotCreateFile = '2';

     /*Writes opcode A to client*/
     nw = write(sd, &opcodeA, sizeof(opcodeA));




     /*Checks if filename exists*/
     if (access(fileName, F_OK) == 0)
     {
          nw = write(sd, &ackfileExists, sizeof(ackfileExists)); //writes ackfileExists to client
          return;
     }

     /*Checks if file can be created*/
     if ((file = fopen(fileName, "wb")) == NULL)
     {
          nw = write(sd, &ackCannotCreateFile, sizeof(ackCannotCreateFile)); //writes ackCannotCreateFile to client
          fclose(file);
          return;
     }
     /*If no issues, write success ack*/
     nw = write(sd, &ackSuccess, sizeof(ackSuccess));





     /*read opcode B from client and continue if it is received*/
     nr = read(sd, &opcodeB, sizeof(opcodeB));

     if (opcodeB == 'B')
     {
          /*Reads file size*/
          nr = read(sd, &fileSize, sizeof(fileSize));
          int normalfileSize = ntohl(fileSize);
          fileSize = normalfileSize;

          /*Reads file data*/
          int tempChunkSize = CHUNK_SIZE;
          if (CHUNK_SIZE > fileSize)
               tempChunkSize = fileSize;

          char buf[tempChunkSize];
          int remainingBytes = fileSize;

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
          fclose(file);
     }
}

void serve_get(int sd)
{
     int nr, nw;
     //Read the file name lenght from the Client
     char fileName[32];
     bzero(fileName, sizeof(fileName));
     int fileNameLen;

     read(sd, &fileNameLen, sizeof(fileNameLen));
     fileNameLen = ntohl(fileNameLen);
     read(sd, fileName, fileNameLen);

     /*Defines OPCODES and ACKCODES*/
     char opcodeA = 'A', opcodeB, ackSuccess = '0', ackfileDoesNotExist = '1';

     write(sd, &opcodeA, sizeof(opcodeA));

     if (access(fileName, F_OK) == 0)
     {
          nw = write(sd, &ackSuccess, sizeof(ackSuccess)); //writes ackSuccess to client
          //Server sends the file size
          int fileSize = getFileSize(fileName);
          int convertedFileSize = htonl(fileSize);
          write(sd, &convertedFileSize, sizeof(convertedFileSize));

          //Reads file into file and writes data to Client
          FILE *file;
          int nr = 0;
          char buf[CHUNK_SIZE];
          file = fopen(fileName, "rb");
          while ((nr = fread(buf, 1, CHUNK_SIZE, file)) > 0)
          {
               if (write(sd, buf, nr) == -1)
               {
                    printf("failed to send file content\n");
                    return;
               }
          }
          printf("GET SUCCESSFUL\n");
     }
     else
     {
          nw = write(sd, &ackfileDoesNotExist, sizeof(ackfileDoesNotExist)); //writes ackfileDoesNotExist to client
          printf("FILE DOES NOT EXIST\n");
     }
}

void serve_a_client(int sd)
{
     char opcode;

     /*Reads OPCODES and runs appropriate functions*/
     while (read(sd, &opcode, sizeof(opcode)))
     {

          if (opcode == 'P')
          {
               serve_put(sd);
          }
          else if (opcode == 'G')
          {
               serve_get(sd);
          }
          else if (opcode == 'Q')
          {
               break;
          }
     }

     return;
}
int main()
{
     int sd, nsd, n, cli_addrlen;
     pid_t pid;
     struct sockaddr_in ser_addr, cli_addr;

     /* turn the program into a daemon */
     daemon_init();

     /* set up listening socket sd */
     if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
     {
          perror("server:socket");
          exit(1);
     }

     /* build server Internet socket address */
     bzero((char *)&ser_addr, sizeof(ser_addr));
     ser_addr.sin_family = AF_INET;
     ser_addr.sin_port = htons(SERV_TCP_PORT);
     ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     /* note: accept client request sent to any one of the
        network interface(s) on this host. 
     */

     /* bind server address to socket sd */
     if (bind(sd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
     {
          perror("server bind");
          exit(1);
     }

     /* become a listening socket */
     listen(sd, 5);

     while (1)
     {

          /* wait to accept a client request for connection */
          cli_addrlen = sizeof(cli_addr);
          nsd = accept(sd, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
          if (nsd < 0)
          {
               if (errno == EINTR) /* if interrupted by SIGCHLD */
                    continue;
               printf("Knee grow\n");
               perror("server:accept");

               exit(1);
          }

          /* create a child process to handle this client */
          if ((pid = fork()) < 0)
          {
               perror("fork");
               exit(1);
          }
          else if (pid > 0)
          {
               close(nsd);
               continue; /* parent to wait for next client */
          }

          /* now in child, serve the current client */

          close(sd);
          //menuDisplay(nsd,n);
          serve_a_client(nsd);
          exit(0);
     }
}
