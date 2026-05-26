/**

    getaddrinfo
    in loop - socket and connect
    recv
    close(socketfd)


    NOTE - any side can use send or recv
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "3490"
#define MAXDATASIZE 100

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        // sinaddr from sockaddr_in
        return &((struct sockaddr_in *)sa)->sin_addr;
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int socketfd, numbytes;

    struct addrinfo hints, *servinfo, *p;

    char buf[MAXDATASIZE];

    char s[INET6_ADDRSTRLEN];

    int rv;  // will contain the status returned from getaddrinfo

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        // try to get the socketfd
        if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;  // check for the next node
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)&p->ai_addr), s, sizeof s);

        printf("client: attempting connection to %s\n", s);

        if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(socketfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect \n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client connected to %s\n", s);

    freeaddrinfo(servinfo);

    if ((numbytes = recv(socketfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    printf("client: received '%s'\n", buf);

    close(socketfd);

    return 0;
}