/*
   THIS SERVER will send "Hello World" out oer a stream connection
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "3490"
#define BACKLOG 10

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        // sinaddr from sockaddr_in
        return &((struct sockaddr_in *)sa)->sin_addr;
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main() {
    int socketfd, newfd;

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;  // connector's address info

    socklen_t sin_size;

    struct sigaction sa;

    int yes = 1;

    char s[INET6_ADDRSTRLEN];

    int rv;  // will contain the status returned from getaddrinfo

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // bind my own ip

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can

    for (p = servinfo; p != NULL; p = p->ai_next) {
        // try to get the socketfd
        if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;  // check for the next node
        }

        /** This part is not known to me .. next 4 lines*/
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // bind the port
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
            // close the current socketfd as we create a new one again in next iteration
            close(socketfd);
            perror("server: bind");  // try to bind with the next
            continue;
        }

        // If we are here that means we have socketfd and bind thingy done for us
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        // it means we did not succeed in binding
        fprintf(stderr, "server: failed to bind \n");
        exit(1);
    }

    if (listen(socketfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    /** This part is not known to me .. next 8 lines*/
    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    // accept loop

    while (1) {
        sin_size = sizeof their_addr;
        newfd = accept(socketfd, (struct sockaddr *)&their_addr, &sin_size);

        if (newfd == -1) {
            perror("accept");
            continue;
        }

        // find out the connected source ip
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        printf("server got connection from %s\n", s);

        if (!fork()) {        // this is the child process
            close(socketfd);  // child does not need listener
            if (send(newfd, "Hello World!", 13, 0) == -1) {
                perror("send");
            }
            close(newfd);
            exit(0);
        }

        close(newfd);  // parent do no need this
    }
    return 0;
}