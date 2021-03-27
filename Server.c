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

#define   BUFSIZE         256
#define   SERV_TCP_PORT   40001           /* server port no */

void claim_children()
{
     pid_t pid=1;
     
     while (pid>0) { /* claim as many zombies as we can */
         pid = waitpid(0, (int *)0, WNOHANG); 
     } 
}

void daemon_init(void)
{       
     pid_t   pid;
     struct sigaction act;

     if ( (pid = fork()) < 0) {
          perror("fork"); exit(1); 
     } else if (pid > 0) 
          exit(0);                  /* parent terminates */

     /* child continues */
     setsid();                      /* become session leader */
     chdir("/");                    /* change working directory */
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


void serve_a_client(int sd)
{   int nr, nw;
    char buf[BUFSIZE];

    while (1){ 
         /* read data from client */
          if ((nr = read(sd, buf, sizeof(buf))) <= 0) 
             exit(0);   /* connection broken down */

         /* send results to client */
         nw = write(sd, buf, nr);
    } 
}  

 
/*
void menuDisplay(int sd, int n)  
{ 
     char choice; 

     n=write(sd, "Enter an option a b c or whatever", sizeof(char)); 
     if(n<0) { 
          printf("error writing to Client"); 
     } 
     read(sd, &choice, sizeof(BUFSIZE));  
     scanf("%s" ,&choice); 
     printf("Client:  %s" ,&choice); 
} 
*/ 

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
     }
}
