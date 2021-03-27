/* cli5.c - 	For ICT374 Topic 8
 *              Hong Xie
 *              Last modfied: 16/10/2020
 *		A much improved (still imperfect) version of "cli4.c".
 */

#include  <unistd.h>        
#include  <stdlib.h>        
#include  <sys/types.h>        
#include  <sys/socket.h>
#include  <netinet/in.h>       /* struct sockaddr_in, htons, htonl */
#include  <netdb.h>            /* struct hostent, gethostbyname() */ 
#include  <string.h>
#include  <stdio.h>

#define   BUFSIZE        256
#define   SERV_TCP_PORT  40001 /* server's "well-known" port number */

int main(int argc, char *argv[])
{
     int sd, n, nr, nw;
     char buf[BUFSIZE], host[BUFSIZE];
     struct sockaddr_in ser_addr; struct hostent *hp;

     /* get server host name */
     if (argc==1)  /* assume server running on the local host */
         strcpy(host, "localhost");
     else if (argc == 2) /* use the given host name */ 
         strcpy(host, argv[1]);
     else {
         printf("Usage: %s [<server_host_name>]\n", argv[0]); exit(1); 
     }

    /* get host address, & build a server socket address */
     bzero((char *) &ser_addr, sizeof(ser_addr));
     ser_addr.sin_family = AF_INET;
     ser_addr.sin_port = htons(SERV_TCP_PORT);
     if ((hp = gethostbyname(host)) == NULL){
           printf("host %s not found\n", host); exit(1);   
     } 

     ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;

     /* create TCP socket & connect socket to server address */
     sd = socket(PF_INET, SOCK_STREAM, 0);
     if (connect(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0) { 
          perror("client connect"); exit(1);
     }
    
     while (1)  
     { 
          
          /*char choice; 
          bzero(buf,BUFSIZ);
          nr=read(sd, buf,BUFSIZ);  
          
          if(nr < 0)  
          { 
               printf("Error reading from the server"); 
          } 
          printf("Server : %s\n" ,buf);
          write(sd,&choice,sizeof(char));  
     */
          printf("Client Input: ");
          fgets(buf, BUFSIZE, stdin); nr = strlen(buf); 
          if (buf[nr-1] == '\n') { buf[nr-1] = '\0'; --nr; }

          if (strcmp(buf, "quit")==0)  
          {
               printf("Bye from client\n"); exit(0);
          }

          if (nr > 0)  
          {
               nw = write(sd, buf, nr);
               nr = read(sd, buf, BUFSIZE); buf[nr] = '\0';
               printf("Sever Output[%d]: %s\n", i, buf);
          } 
     }
}
