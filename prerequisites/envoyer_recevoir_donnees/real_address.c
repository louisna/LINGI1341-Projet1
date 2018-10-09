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

// Avec aide de Florian Blondiaux

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