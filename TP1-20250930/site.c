#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[4096];

    char *hostname = "peaks.fr"; // serveur à lire
    int port = 80;

    // Résolution DNS
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Erreur: impossible de résoudre le host\n");
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET pour IPv4 et SOCK_STREAM pour TCP
    if (sockfd < 0) { perror("socket"); exit(0); }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(0);
    }

    printf("Connecté à %s\n", hostname);

        char request[512];
    snprintf(request, sizeof(request),
             "GET / HTTP/1.0\r\nHost: %s\r\n\r\n", hostname);

    if (write(sockfd, request, strlen(request)) < 0) {
        perror("write");
        exit(0);
    }

        int n;
    memset(buffer, 0, sizeof(buffer));
    while ((n = read(sockfd, buffer, sizeof(buffer)-1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);  // affiche tout le HTML
    }

        char *start = strstr(buffer, "<title>");
    if (start) {
        start += strlen("<title>");
        char *end = strstr(start, "</title>");
        if (end) {
            *end = '\0';  // termine la chaîne à </title>
            printf("\nTitre de la page : %s\n", start);
        }
    }

        close(sockfd);
    return 0;
}
