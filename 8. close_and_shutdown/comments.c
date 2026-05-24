/*
    CLOSE AND SHUTDOWN

    - you can just use regular UNIX FD close() function to close the connection.. close(socketfd)

      - this will prevent more read and writes to the socket.
        Anyone attempting to read or write the socket on the remote end will receive an error

        -- To get a little more control over how the socket closes, you can use shutdown() function
           - allows you to cut of communication in a certain direction or both ways (like close() )

           int shutdown(int sockfd, int how);
           - sockfd - socket fd you want to shutdown
           - how
               0 - further receives are not allowed
               1 - further sends are not allowed
               2 - further sends andd receives are not allowed ( close() )

          shutdown() returns 0 on success .. -1 on error (errno)

          *****
          IMPORTANT - shutdown just changes the usability of fd .. it does not close the fd
          To free socket fd you need to call close()

          closesocket() - in windows


*/

/**

getpeername() - Who are you
- it will tell you who is at the other end of the connected STREAM SOCKET
  the synopsis

  #include <sys/socket.h>
  int getpeername(int socketfd, struct sockaddr *addr, int *addrlen)

  - sockfd - fd of connected stream socket
  - addr - pointer to struct sockaddr ( or a struct sockaddr_in) that will hold the info
           about the other side of the connection
  - addrlen - pointer to an integer that should be initialiazed to sizeof *addr or
              sizeof( struct sockaddr )

   The function returns 0 on success .. -1 on error (errno)

   Once you have their address , you can use inet_ntop(), getnameinfo(), or gethostbyaddr()
   to print or get more info. .. can not get their login name unless the other computer is
   running on ident daemon( no idea what tf this shit means ). RFC 1413

*/

/***

    gethostbyname() - who am I

    returns the name of the computer that your program is running on . the name can then be used
    by getaddrinfo(), to determine IP address of the local machine.

    #include<unistd.h>
    int gethostname(char *hostname, size_t size);

    - hostname - pointer to an array of chars that will contain the hostname upon the function's
                 return
    - size - length in bytes of the hostname array

    The function returns 0 on success .. -1 on error (set errno)
*/