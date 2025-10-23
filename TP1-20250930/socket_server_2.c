/* Serveur sockets TCP
 * affichage de ce qui arrive sur la socket
 *    socket_server port (port > 1024 sauf root)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>


int main(int argc, char** argv )
{ char datas[] = "hello\n";
  int    sockfd,scomm, pid;
  socklen_t clilen;
  struct sockaddr_in cli_addr,serv_addr;
  char c;
  

  if (argc!=2) {printf ("usage: socket_server port\n");exit(0);}
 
  printf ("server starting...\n");  
  
  /* ouverture du socket */
  sockfd = socket (AF_INET,SOCK_STREAM,0);
  if (sockfd<0) {printf ("impossible d'ouvrir le socket\n");exit(0);}

  /* initialisation */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

  /* effecture le bind */
  if (bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
     {printf ("impossible de faire le bind\n");exit(0);}

  /* écoute */
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    signal(SIGCHLD,SIG_IGN); /* on ignore les fils morts */
     while (1) {
        clilen = sizeof(cli_addr);
        scomm = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (scomm < 0) {
            perror("accept");
            continue;
        }
        printf("Nouvelle connexion acceptée\n");

        pid = fork();
        if (pid < 0) {
            perror("fork");
            close(scomm);
            continue;
        }

        if (pid == 0) {  /* Processus fils */
            close(sockfd); /* le fils n’a pas besoin du socket d’écoute */

            /* Communication avec le client */
            while (read(scomm, &c, 1) > 0) {
                printf("Reçu : %c\n", c);
                if (write(scomm, &c, 1) < 0) break;  // echo au client
            }

            close(scomm);
            exit(0);  /* on tue le fils après la fin de la comm */
        } else {
            /* Processus père : il n’a pas besoin du scomm */
            close(scomm);
        }
    }

    close(sockfd);
    return 0;
 }
