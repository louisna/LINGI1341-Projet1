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
#include "packet_implement.h"


/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval);

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port,
                 struct sockaddr_in6 *dest_addr, int dst_port);

typedef struct node{
	pkt_t* packet;
	struct node* next;
} node_t;

typedef struct linked_list{
	node_t* head;
	node_t* tail;
	int size;
} list_t;

/*
 * Add a packet at the tail of the list and updates the size and the tail
 * @list: the list to be manipulated, list != NULL
 * @packet: the packet to be added to the tail of the list
 * @return: -1 in case of error, 0 otherwise
 */
int add_element_queue(list_t* list, pkt_t* packet);

