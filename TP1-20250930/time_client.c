/* Client pour les sockets
 *    socket_client ip_server port
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
int main(int argc, char** argv )
{ 
  int    sockfd,newsockfd,clilen,chilpid,ok,nleft,nbwriten;
  uint32_t net_time;
  unsigned long int host_time;
  struct sockaddr_in cli_addr,serv_addr;

  if (argc!=3) {printf ("usage  socket_client server port\n");exit(0);}
 
 
  /*
   *  partie client 
   */
  printf ("client starting\n");  

  /* initialise la structure de donnee */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family       = AF_INET;
  serv_addr.sin_addr.s_addr  = inet_addr(argv[1]);
  serv_addr.sin_port         = htons(atoi(argv[2]));
  
  /* ouvre le socket */
  if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
    {printf("socket error\n");exit(0);}
  
  /* effectue la connection */
  if (connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
    {printf("socket error\n");exit(0);}
    
  
   // Lire 4 octets du serveur
    if (read(sockfd, &net_time, sizeof(net_time)) != sizeof(net_time)) {
        perror("read");
        close(sockfd);
        exit(1);
    }

    close(sockfd);

    // Convertir en ordre machine
    host_time = ntohl(net_time);

    // Ajuster : protocole TIME compte depuis 1900, time_t depuis 1970
    host_time -= 2208988800U;

    // Afficher la date
    time_t t = (time_t)host_time;
    printf("Heure reçue : %s", ctime(&t));

    return 0;

}
