#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "9034"

const char *inet_ntop2(void *addr, char *buf, size_t size) {
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family) {
        case AF_INET:
            sa4 = addr;
            src = &(sa4->sin_addr);
            break;
        case AF_INET6:
            sa6 = addr;
            src = &(sa6->sin6_addr);
            break;
        default:
            return NULL;
    }
    return inet_ntop(sas->ss_family, src, buf, size);
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count) {
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];
    (*fd_count)--;
}

void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size) {
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2;  // Double it
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;  // Check ready-to-read
    (*pfds)[*fd_count].revents = 0;
    (*fd_count)++;
}

void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i) {
    char buf[256];  // Buffer for client data
    int nbytes = recv(pfds[*pfd_i].fd, buf, sizeof buf, 0);
    int sender_fd = pfds[*pfd_i].fd;

    if (nbytes <= 0) {  // Got error or connection closed by client
        if (nbytes == 0) {
            // Connection closed
            printf("pollserver: socket %d hung up\n", sender_fd);
        } else {
            perror("recv");
        }

        close(pfds[*pfd_i].fd);  // Bye!

        del_from_pfds(pfds, *pfd_i, fd_count);
        // reexamine the slot we just deleted
        (*pfd_i)--;

    } else {  // We got some good data from a client

        printf("pollserver: recv from fd %d: %.*s", sender_fd, nbytes, buf);

        // Send to everyone!
        for (int j = 0; j < *fd_count; j++) {
            int dest_fd = pfds[j].fd;

            // Except the listener and ourselves
            if (dest_fd != listener && dest_fd != sender_fd) {
                if (send(dest_fd, buf, nbytes, 0) == -1) {
                    perror("send");
                }
            }
        }
    }
}

void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds) {
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;

    int newfd;  // newly accepted socket fd

    char remoteIP[INET6_ADDRSTRLEN];

    addrlen = sizeof remoteaddr;

    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

    if (newfd == -1) {
        perror("accept");
    } else {
        add_to_pfds(pfds, newfd, fd_count, fd_size);

        printf("pollserver new connection from %s on socket %d\n",
               inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP), newfd);
    }
}

void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds) {
    for (int i = 0; i < *fd_count; i++) {
        // check if someone is ready to read
        if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {
            if ((*pfds)[i].fd == listener) {
                // if this is a listener .. means we are a new connection
                handle_new_connection(listener, fd_count, fd_size, pfds);
            } else {
                // we are just a regulat client
                handle_client_data(listener, fd_count, *pfds, &i);
            }
        }
    }
}

int get_listener_socket() {
    int listener;
    int yes = 1;
    int rv;

    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (listener < 0) {
            continue;
        }

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    if (p == NULL) {
        return -1;
    }

    if (listen(listener, 10) == -1) {
        close(listener);
        return -1;
    }

    return listener;
}

int main() {
    int listener;  // listening fd (socket)

    // room for 5 connections
    int fd_size = 5;
    int fd_count = 0;

    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    pfds[0].fd = listener;
    pfds[0].events = POLLIN;

    fd_count = 1;

    puts("pollserver: waiting for connections ...");

    while (1) {
        int poll_count = poll(pfds, fd_count, -1);  // so the timeout here is set to -1

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        process_connections(listener, &fd_count, &fd_size, &pfds);
    }

    free(pfds);
}