// THIS CHAPTER IS ALL ABOUT listen()
/**

   Using listen we can wait for incoming connections and handle them in some way.
   This process is two step
   1. listen()
   2. accept() .. will see in next chapter

   // syntax for the listen()

   int listen(int socketfd, int backlog)

   What is backlog ? - Number of connections allowed on the incoming queue
   - incoming connections are going to wait in this queue until you accept() them.
   - backlog is the limit on how many can queue up

   // Most systems silently limit this number to about 20

    listen() returns -1 on error and sets errno on error


    === IMPORTANT ===
    We need to call bind() before listen() so that the server is running on a specific port.

    // SEQUENCE
    - getaddrinfo()
    - socket()
    - bind()
    - listen()

    ---- then comes the accept()
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT "3000"
int main() {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // bind my own ip using AI PASSIVE
    hints.ai_flags = AI_PASSIVE;

    // now to use any port pass that to the getaddrinfo
    int status;

    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo failed: %s \n", gai_strerror(status));
        return 1;
    }

    // ofc again I am mentioning here that we should traverse the linked list
    // pointed by the res for the better results
    // here I am assuming that the first node has the correct data .. for more
    // info see the first directory in this repo

    // Tired of this comment rn .. won't write this from now onwards

    int socketfd;

    if ((socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        // means error aagya
        fprintf(stderr, "socket() failed : %s\n", strerror(errno));
        return 2;
    }

    // now we have the socket fd .. so use it for binding

    // int bind (int socketfd, struct sockaddr *my_addr, int addrlen)
    int bindstatus;
    if ((bindstatus = bind(socketfd, res->ai_addr, res->ai_addrlen)) == -1) {
        // bind nhi ho paya
        fprintf(stderr, "bind() failed: %s\n", strerror(errno));
        return 3;
    }

    // NOW FINALLY listen()

    listen(socketfd, 20);
    // ofc you can handle error ..
    return 0;
}

// This was fairly simple .. let's look into the accept in the next directory