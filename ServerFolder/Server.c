#include <stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <fcntl.h>
#include<errno.h>

#include  <unistd.h>
#include  <stdlib.h>
#include  <stdio.h>
#include  <sys/stat.h>
#include  <string.h>     /* strlen(), strcmp() etc */
#include  <errno.h>      /* extern int errno, EINTR, perror() */
#include  <signal.h>     /* SIGCHLD, sigaction() */
#include  <sys/types.h>  /* pid_t, u_long, u_short */
#include  <sys/socket.h> /* struct sockaddr, socket(), etc */
#include  <sys/wait.h>   /* waitpid(), WNOHAND */
#include  <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
                         /* and INADDR_ANY */

#define   BUFSIZE         200000
#define   SERV_TCP_PORT   40001           /* server port no */
#define   FILE_BLOCK_SIZE   512           /* server port no */

void claim_children()
{
     pid_t pid=1;
     
     while (pid>0) { /* claim as many zombies as we can */
         pid = waitpid(0, (int *)0, WNOHANG); 
     } 
}

int read_nbytes(int sd, char *buf, int nbytes)
{
	int nr = 1;
	int n = 0;
	for (n=0; (n < nbytes) && (nr > 0); n += nr) {
		if ((nr = read(sd, buf+n, nbytes-n)) < 0){
			return (nr); /* read error */
		}
	}
	return (n);
}
void daemon_init(void)
{       
     pid_t   pid;
     struct sigaction act;

     if ( (pid = fork()) < 0) {
          perror("fork"); exit(1); 
     } else if (pid > 0){
          printf("PID: %d\n", pid); 
          exit(0);                  /* parent terminates */
     }
     /* child continues */
     setsid();                      /* become session leader */
     //chdir("/");                    /* change working directory */
     umask(0);                      /* clear file mode creation mask */

     /* catch SIGCHLD to remove zombies from system */
     act.sa_handler = claim_children; /* use reliable signal */
     sigemptyset(&act.sa_mask);       /* not to block other signals */
     act.sa_flags   = SA_NOCLDSTOP;   /* not catch stopped children */
     sigaction(SIGCHLD,(struct sigaction *)&act,(struct sigaction *)0);
     /* note: a less than perfect method is to use 
              signal(SIGCHLD, claim_children);
    */
}

void serve_put(int sd){
    int nr, nw, i=0;
    char buf[BUFSIZE];
    char fileName[32];
    int fileSize = 0, fileNameSize = 0;
    FILE *file;


     /*Reads file name size*/
     nr = read(sd, &fileNameSize, sizeof(fileNameSize));
     fileNameSize = ntohl(fileNameSize);

     /*Reads file size*/
     nr = read(sd, &fileSize, sizeof(fileSize));  
     int normalFileSize = ntohl(fileSize);
     fileSize = normalFileSize;
     
     /*Reads name file size*/
     nr = read(sd, fileName, fileNameSize);
     
     /*Reads file data*/
     nr = read(sd, buf, fileSize);
     
     /*Writes data to the file*/
     file = fopen(fileName, "wb");
     fwrite(buf, fileSize, sizeof(unsigned char), file);
     fclose(file);
}

void serve_a_client(int sd){
  char opcode;

  /*Reads OPCODES and runs appropriate functions*/
  while(read(sd, &opcode, sizeof(opcode))){
       printf("OPCODE IS: %c", opcode);
       if(opcode == 'P'){
          serve_put(sd);
       }else if(opcode == 'Q'){
            break;
       }
  }
  printf("SERVED CLIENT\n");
  return;


}
int main()
{
     int sd, nsd, n, cli_addrlen;  pid_t pid;
     struct sockaddr_in ser_addr, cli_addr; 
     
     /* turn the program into a daemon */
     daemon_init();  

     /* set up listening socket sd */
     if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
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
     if (bind(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0){
          perror("server bind");  
          exit(1);
     }

     /* become a listening socket */
     listen(sd, 5);

     while (1) {

          /* wait to accept a client request for connection */
          cli_addrlen = sizeof(cli_addr);
          nsd = accept(sd, (struct sockaddr *) &cli_addr, (socklen_t *)&cli_addrlen);
          if (nsd < 0)  
          {
               if (errno == EINTR)   /* if interrupted by SIGCHLD */
                    continue;
               printf("Knee grow\n");
               perror("server:accept");  
               
               exit(1);
          } 

          /* create a child process to handle this client */
          if ((pid=fork()) <0) {
              perror("fork"); exit(1);
          } else if (pid > 0) { 
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
