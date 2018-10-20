#include "nyancat.h"

int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){

    int fd_s = socket(AF_INET6, SOCK_DGRAM, 0);
    if(fd_s == -1){
        fprintf(stderr, "socket returned a -1 descriptor\n");
        return -1;
    }

    if(source_addr != NULL){
        source_addr->sin6_port = htons(src_port);
        int error = bind(fd_s, (struct sockaddr*) source_addr, sizeof(struct sockaddr_in6));
        if(error == -1){
            fprintf(stderr, "bind returned -1 \n");
            return -1;
        }
    }

    if(dest_addr != NULL){
        dest_addr->sin6_port = htons(dst_port);
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

list_t* list_create(){
    list_t* list = (list_t*)calloc(1, sizeof(list_t));
    if(!list){
        fprintf(stderr, "Not enough memory [list_create]\n");
        return NULL;
    }
    return list;
}
/*
lack_t* lack_create(){
    lack_t* lack = (lack_t*)calloc(1, sizeof(lack_t));
    if(!lack){
        fprintf(stderr, "Not enough memory [lack_create]\n");
        return NULL;
    }
    return lack;
}
*/

//longueur max de queue ?
int add_element_queue(list_t* list, pkt_t* packet){

    node_t * new_node = (node_t*) malloc(sizeof(node_t));
    if(new_node==NULL){
        fprintf(stderr, "Not enough memory [add element queue]\n");
        return -1;
    }
    
    new_node->packet = packet;
    new_node->next = NULL;
    
    if(list->size==0){//liste vide
        list->head = new_node;
        list->tail = new_node;
        list->size++;
        return 0;
    }
    else{//liste pas vide
        node_t * tail = list->tail;
        tail->next = new_node;
        list->tail = new_node;
        list->size++;
        return 0;
    }
}

pkt_t* pop_element_queue(list_t* list){
    if(list->size==0){
        fprintf(stderr, "Empty list [pop element queue]\n");
        return NULL;
    }
    else{
        node_t * head = list->head;
        pkt_t* p = head->packet;

        if(list->size==1){
            list->head = NULL;
            list->tail = NULL;
            list->size--;
        }
        else{
            list->head = list->head->next;
            list->size--;
        }
        free(head);
        return p;
    }
}

int add_specific_queue(list_t* list, pkt_t* packet){
    node_t * new_node = (node_t*) malloc(sizeof(node_t));
    if(new_node==NULL){
        fprintf(stderr, "Not enough memory [add element queue]\n");
        return -1;
    }
    
    new_node->packet = packet;
    new_node->next = NULL;
    
    if(list->size==0){//liste vide
        list->head = new_node;
        list->tail = new_node;
        list->size++;
        return 0;
    }
    else{//liste pas vide
        int seqnum = pkt_get_seqnum(packet);
        node_t* runner = list->head;
        pkt_t* pkt_runner = runner->packet;
        node_t* previous = NULL;
        
        while(runner != NULL && pkt_get_seqnum(pkt_runner) < seqnum){ // PREND PAS EN COMPTE MODULO
            previous = runner;
            runner = runner->next;
            if(runner != NULL)
                pkt_runner = runner->packet;
        }
        if(pkt_get_seqnum(pkt_runner) == seqnum){
            pkt_del(packet);
            free(new_node);
            //alreadyin buffer
            return 0;
        }
        if(previous == NULL){
            list->head = new_node;
        }
        else{
            previous->next = new_node;
        }
        new_node->next = runner;
        if(runner == NULL)
            list->tail = new_node;
        list->size++;
        return 0;
    }
}