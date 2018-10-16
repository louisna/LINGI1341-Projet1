#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>        /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h> 
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include "nyancat.h"
#include "packet_implement.h"

#define MAX_READ_SIZE 1024 // need to be changed ?

/*
 * Sends the packet pkt to be an ack
 * @pkt: the packet to be sent
 * @sfd: the socket file descriptor
 * @return 0 in case of success, 0 otherwise
 */
int send_ack(pkt_t* pkt, int sfd){
	time_t current_time = time(NULL);	
	uint32_t  a_lo = (uint32_t) current_time;

	pkt_status_code err1 = pkt_set_timestamp(pkt, a_lo);
	if(err1!=PKT_OK){
		fprintf(stderr, "Error while encoding time in packet : error is %d\n",err1);
		return -1;
	}

	size_t length = 3*sizeof(uint32_t);
	// length should be 0
	if(pkt_get_length(pkt)!=0){
		length += pkt_get_length(pkt);
		length += sizeof(uint32_t);
	}
	char buff[length];
				
	pkt_status_code err2 = pkt_encode(pkt, &buff, &length);
	if(err2!=PKT_OK){
		fprintf(stderr, "Error while using pkt_encode : error is %d\n",err2);
		return -1;
	}

	ssize_t err3 = send(sfd, &buff, length,0);
	if(err3==-1){
		fprintf(stderr, "Error while sending packet\n");
		return -1;
	}
	return 0;
}

/*
 * Computes an ack with a seqnum, a type and a window
 * @return: the new pkt, NULL in case of error
 */
pkt_t* create_ack(int seqnum, int type, int new_window){
	pkt_t* new = pkt_new();
	if(!new){
		return NULL;
	}
	pkt_set_type(new, type);
	pkt_set_seqnum(new, seqnum);
	pkt_set_payload(new, NULL, 0);
	pkt_set_window(new, new_window);
	return new;

}
/*
 * Givent list and first_seqnum, checks and create an ack for
 * each paket with a consecutive seqnum
 * @list: the list
 * @first_seqnum: the first seqnum of the ack
 * @sfd: the socket file descriptor
 * @return the seqnum of the last ack sent
 */
int check_for_ack(list_t* list, int first_seqnum, int sfd){
	node_t* runner = list->head;
	pkt_t* pkt = runner->packet;
	int previous = pkt_get_seqnum(pkt);
	if(previous > first_seqnum){ // or first_seqnum + 1 ?
		fprintf(stderr, "We lost the good ack... problem\n");
		return 0; // we still have to wait for the last ack
	}
	while(runner != NULL){
		pkt_t* pkt = runner->packet;
		int actual = pkt_get_seqnum(pkt);
		if((actual == 0 && previous == 255) || actual - first_seqnum <= 1){
			// seqnum of the pkt is the extactly next of the previous
			// we can send an ack for this pkt
			pkt_t* ack;
			int new_window = MAX_WINDOW_SIZE - list->size;
			if(pkt_get_tr(pkt)){
				ack = create_ack(actual, PTYPE_NACK, new_window);
			}
			else{
				ack = create_ack(actual, PTYPE_ACK, new_window);
			}
			int err = send_ack(pkt, sfd);
			if(err){
				fprintf(stderr, "Impossible to send the ack %d\n", pkt_get_seqnum(pkt));
			}
			pkt_del(ack);
			runner = runner->next;
			first_seqnum++;
		}
		else{
			return first_seqnum;
		}
	}
	return first_seqnum;
}

/*
 * Free all the packets/nodes with a seqnum below the seqnum
 * @list: the list
 * @seqnum: the seqnum of the last ack (which must be keeped)
 */
void free_packet_queue(list_t* list, int seqnum){
	node_t* runner = list->head;
	while(runner != NULL){
		pkt_t* pkt = runner->packet;
		int seq = pkt_get_seqnum(pkt);
		if(seq >= seqnum){
			return;
		}
		pkt_t* pkt2;
		int err = pop_element_queue(list, pkt2);
		if(err){
			fprintf(stderr, "Erreur [free_packet_queue]\n");
		}
		pkt_del(pkt2);
	}
}

/*
 * Reads data from the sfd, and add the packets to the list
 * @list: the list
 * @window: a pointer to update the window size
 * @seqnum: the seqnum of the first packet waited
 * @return: -1 in case of error, -2 if the transfer is done, -1 otherwise
 */
int read_to_list_r(list_t* list, int *window, int seqnum, int sfd){
	if(!list){
		fprintf(stderr, "BIG ERROR: list NULL!\n");
		return -1;
	}
	while(list->size < window){
		char buffer[MAX_PAYLOAD_SIZE];
		int readed = recv(sfd, buffer, MAX_READ_SIZE, 0);
		if(readed == -1){
			fprintf(stderr, "Error while receving data [read_to_list_r]\n");
		}
		else{
			pkt_t* pkt = pkt_new();
			if(!pkt){
				fprintf(stderr, "Not enough memory while creation a packet [read_to_list_t]\n");
			}
			else{
				int err = pkt_decode(buffer, readed, pkt);
				if(err){
					fprintf(stderr, "Impossible to decode [read___r]\n");
				}

				if(seqnum == pkt_get_seqnum(pkt) && pkt_get_length(pkt) == 0){
					//end
					return -2;
				}
				*window = pkt_get_window(pkt);
				int seqnum_interval_max = (seqnum + *window) % 256;
				if(seqnum_interval_max > pkt_get_seqnum(pkt)){ // A CHANGER !!! PREND PAS EN COMPTE LE MODULO
					// peut etre ajoute
					add_specific_queue(list, pkt);
				}
			}
		}
	}
	return 0;
}
/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){
    char buffer[1024];
    
    struct sockaddr_in6 sock;
    socklen_t len = sizeof(sock);

    int nread = recvfrom(sfd, buffer, sizeof(char)*1024, MSG_PEEK, (struct sockaddr*) &sock, &len);
    if(nread == -1){
        fprintf(stderr, "Error using recvfrom\n");
        return -1;
    }

    int done = connect(sfd, (struct sockaddr*) &sock, (int) len);
    if(done == -1){
        fprintf(stderr, "Error using connect");
        return -1;
    }

    return 0;
}

/*
 * Global process
 */
int process_receiver(int sfd, int fileOut){
	fd_set check_fd; // for the function select()
	int retval; // return value of select
	list_t* list = list_create();
	int seqnum = 0;
	int current_window = 1;
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if(!list){
		fprintf(stderr, "Not enough memory to create the list. Fatal error.\n");
		exit(EXIT_FAILURE);
	}

	// check the maximum fd, may be fileOut ?
	int max_fd = sfd > fileOut ? sfd : fileOut;

	while(1){

		FD_ZERO(&check_fd);
		FD_SET(sfd, &check_fd);

		retval = select(max_fd+1, &check_fd, NULL, NULL, &tv);

		if(retval == -1){
			fprintf(stderr, "Error from select [process_receiver]\n");
			return -1; // vraiment ?
		}

		else if(FD_ISSET(sfd, &check_fd)){
			// read from sfd
			// check if in sequence
			// if out of sequence, stock in list
			//	and return the last ack
			// truncated ?
			// if packet length 0 + sequence number already done
			read_to_list_r(list, &current_window, seqnum, sfd);
		}

		seqnum = check_for_ack(list, seqnum, sfd);
		free_packet_queue(list, seqnum);
	}

	return 0;
}

void print_list(list_t* list){
	node_t* runner = list->head;
	while(runner != NULL){
		print_data(runner->packet);
		runner = runner->next;
	}
}

int main(int argc, char* argv[]){

	/* default values */
	int client = 0;
	int port = 12345;
	int opt;
	char* host = "::1";
	char* file = NULL;
	int fd;

	if(argc < 3){
		fprintf(stderr, "The program needs at least 2 arguments to work:\n"
			"- hostname: domain name or IPv6 sender's adress\n"
			"- port: UDP port number where the receiver was plugged\n");
		exit(EXIT_FAILURE);
	}

	while((opt = getopt(argc, argv, "f:")) != -1){
		switch(opt){
			case 'f':
				file = optarg;
				break;
			default:
				fprintf(stderr, "The only accepted option is -f X\n"
					"where X is the name of an existing file\n");
				break;
		}
	}

	if(file && argc < 5){
		fprintf(stderr, "The program needs at least 2 arguments to work:\n"
			"(+ the 2 needed for the file)\n"
			"- hostname: domain name or IPv6 sender's adress\n"
			"- port: UDP port number where the sender was plugged\n");
		exit(EXIT_FAILURE);
	}
	
	host = argv[optind++];
	port = atoi(argv[optind++]);

	/*** Taken from chat.c of the INGInious problem 
	     "envoyer et recevoir des donnes" ***/

	if(file){
		fd = open(file, O_RDWR);
		if(fd < 0){
			fprintf(stderr, "Impossible to open the file %s. Using now stdin\n", file);
			fd = STDOUT_FILENO;
		}
	}
	else{
		fd = STDOUT_FILENO;
	}

	/* Resolve the hostname */
	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if(err){
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		exit(EXIT_FAILURE);
	}

	/*  Get a socket */
	int sfd;
	sfd = create_socket(&addr, port, NULL, -1); /* Bound */
	if(sfd < 0){
		fprintf(stderr, "Failed to create the socket\n");
		exit(EXIT_FAILURE);
	}

	// do something
	int errWait = wait_for_client(sfd);
	if(errWait < 0){
		fprintf(stderr, "Error wait_for_client\n");
		return -1;
	}
	process_receiver(fd, sfd);

	close(sfd);

	return EXIT_SUCCESS;
}
