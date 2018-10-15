#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){

    int fd_s = socket(AF_INET6, SOCK_DGRAM, 0);
    if(fd_s == -1){
        fprintf(stderr, "socket returned a -1 descriptor\n");
        return -1;
    }

    if(source_addr != NULL){
        source_addr->sin6_port = htons(src_port); // why htons ?
        int error = bind(fd_s, (struct sockaddr*) source_addr, sizeof(struct sockaddr_in6));
        if(error == -1){
            fprintf(stderr, "bind returned -1 \n");
            return -1;
        }
    }

    if(dest_addr != NULL){
        dest_addr->sin6_port = htons(dst_port); // why htons ?
        int error = connect(fd_s, (struct sockaddr*) dest_addr, sizeof(struct sockaddr_in6));
        if(error == -1){
            fprintf(stderr, "connect returned -1\n");
            return -1;
        }
    }

    return fd_s; // return the fd !!!
}

const char * real_address(const char *address, struct sockaddr_in6 *rval){
    struct addrinfo *result;
    struct addrinfo hints;


    memset(&hints, 0, sizeof(struct addrinfo)); // All the memory to 0 

    hints.ai_family = AF_INET6;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

    int a = getaddrinfo(address, NULL, &hints, &result);
    if(a != 0){
        return gai_strerror(a); // to compute into a string
    }
    memcpy(rval, result->ai_addr, sizeof(struct sockaddr_in6)); // copying the values to rval
    freeaddrinfo(result); // free the addressinfo after copying
    return NULL; // no error
}