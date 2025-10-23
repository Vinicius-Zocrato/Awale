#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define TIMEOUT_MS 200   /* timeout par port en millisecondes */
#define PORT_START 1
#define PORT_END 65535

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void print_addr(const struct sockaddr *sa, socklen_t salen) {
    char host[NI_MAXHOST];
    if (getnameinfo(sa, salen, host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
        printf("%s", host);
    } else {
        printf("?");
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname_or_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *target = argv[1];
    struct addrinfo hints, *res, *rp;
    int gai;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_family = AF_UNSPEC;     /* IPv4 or IPv6 */

    if ((gai = getaddrinfo(target, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return EXIT_FAILURE;
    }

    /* On choisit la 1ère adresse renvoyée (on utilisera la même famille pour tous les ports) */
    for (rp = res; rp != NULL; rp = rp->ai_next) break;
    if (!rp) {
        fprintf(stderr, "Aucune adresse trouvée pour %s\n", target);
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    printf("Scan de %s (%s) ports %d..%d (timeout %d ms)\n",
           target,
           (rp->ai_family == AF_INET) ? "IPv4" : (rp->ai_family == AF_INET6) ? "IPv6" : "Autre",
           PORT_START, PORT_END, TIMEOUT_MS);

    /* Pour modification locale, on copiera l'adresse dans une sockaddr_storage et modifiera le port */
    struct sockaddr_storage sa_store;
    socklen_t salen;

    for (int port = PORT_START; port <= PORT_END; ++port) {
        /* copie de l'adresse et réglage du port */
        memset(&sa_store, 0, sizeof(sa_store));
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *)&sa_store;
            struct sockaddr_in *src = (struct sockaddr_in *)rp->ai_addr;
            sin->sin_family = AF_INET;
            sin->sin_addr = src->sin_addr;
            sin->sin_port = htons(port);
            salen = sizeof(struct sockaddr_in);
        } else if (rp->ai_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&sa_store;
            struct sockaddr_in6 *src6 = (struct sockaddr_in6 *)rp->ai_addr;
            sin6->sin6_family = AF_INET6;
            sin6->sin6_addr = src6->sin6_addr;
            sin6->sin6_port = htons(port);
            salen = sizeof(struct sockaddr_in6);
        } else {
            continue;
        }

        int fd = socket(rp->ai_family, SOCK_STREAM, 0);
        if (fd < 0) continue;

        if (set_nonblocking(fd) < 0) {
            close(fd);
            continue;
        }

        int ret = connect(fd, (struct sockaddr *)&sa_store, salen);
        if (ret == 0) {
            /* connexion immédiate : port ouvert */
            printf("Port %5d ouvert\n", port);
            close(fd);
            continue;
        }

        if (errno != EINPROGRESS) {
            /* erreur immédiate (refus, unreachable, etc.) */
            close(fd);
            continue;
        }

        /* attente avec select() sur writable */
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);

        struct timeval tv;
        tv.tv_sec = TIMEOUT_MS / 1000;
        tv.tv_usec = (TIMEOUT_MS % 1000) * 1000;

        ret = select(fd + 1, NULL, &wfds, NULL, &tv);
        if (ret > 0 && FD_ISSET(fd, &wfds)) {
            /* vérifier l'erreur socket */
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
                close(fd);
                continue;
            }
            if (so_error == 0) {
                printf("Port %5d ouvert\n", port);
            }
        }

        close(fd);
    }

    freeaddrinfo(res);
    return EXIT_SUCCESS;
}
