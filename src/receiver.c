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
#include <errno.h>
#include <fcntl.h>
#include "nyancat.h"
#include "packet_implement.h"

#define MAX_READ_SIZE 1024 // need to be changed ?

int waited_seqnum = 0; // the waited seqnum
int window_size = 1; // the size of the window
uint32_t last_timestamp = 0;

/*
 * Sends the packet pkt to be an ack
 * @pkt: the packet to be sent
 * @sfd: the socket file descriptor
 * @return 0 in case of success, -1 otherwise
 */
int send_ack(pkt_t* pkt, int sfd, uint32_t timestamp){
	int err1 = pkt_set_timestamp(pkt, timestamp);
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

	pkt_status_code err2 = pkt_encode(pkt, buff, &length);
	if(err2!=PKT_OK){
		fprintf(stderr, "Error while using pkt_encode : error is %d\n",err2);
		return -1;
	}

	ssize_t err3 = send(sfd, buff, length,0);
	if(err3==-1){
		fprintf(stderr, "Error while sending packet\n");
		return -1;
	}
	return 0;
}

/*
 * Computes an ack with a seqnum, a type and a window
 * The window is calculated the following:
 *      MAX_WINDOW_SIZE - list_size
 * @return: the new pkt, NULL in case of error
 */
pkt_t* create_ack(int seqnum, int type, int list_size){
	pkt_t* new = pkt_new();
	if(!new){
		return NULL;
	}
	pkt_set_type(new, type);
	pkt_set_seqnum(new, seqnum);
	pkt_set_payload(new, NULL, 0);
	window_size = MAX_WINDOW_SIZE - list_size;
	pkt_set_window(new, window_size);
	return new;
}

/*
 * Checks if the seqnum from the packet received is in the window
 * of the receiver
 * @return: 1 if seqnum is in the window, 0 otherwise
 */
int check_in_window(int seqnum){
	if(seqnum >= waited_seqnum){ // pas de passage 255 -> 0
		if(seqnum - waited_seqnum <= MAX_WINDOW_SIZE)
			return 1;
		else
			return 0;
	}
	else{//passage par 255->0
		if(255 + seqnum - waited_seqnum <= MAX_WINDOW_SIZE)
			return 1;
		else
			return 0;
	}
}
/*
 * Writes on fd all the packets with the seqnum >= waited_seqnum.
 * The waited_seqnum is updated (+1) when a packet is written on fd.
 * The function then pop the element of the list and free the packet.
 * For each of the packets in sequence, sends an ack with the corresponding seqnum.
 * If the first element of the list has a seqnum > waited_seqnum, it means that
 * the receiver has to wait for at least another packet. Then it sends an ack with the
 * waited seqnum
 * @list: the list
 * @sfd: the socket file drscriptor
 * @fd: the file descriptor to write in
 * @return: 1 if the receiver has to wait for at least 1 packet in sequence, -1 in case of error
 *          -10 if EOF received, 0 otherwise
 */
int write_in_sequence(list_t* list, int sfd, int fd){
	node_t* runner = list->head;
	pkt_t* packet = runner->packet;
	if(runner != NULL && pkt_get_seqnum(packet) > waited_seqnum){
		// not in sequence
		return 1;
	}
	while(runner != NULL){
		packet = runner->packet;
		if(pkt_get_seqnum(packet) == waited_seqnum){
			int writed = write(fd, pkt_get_payload(packet), pkt_get_length(packet));
			if(writed == -1){
				fprintf(stderr, "Impossible to write on fd\n");
				return -1;
			}
			waited_seqnum = (waited_seqnum + 1) % 256;
			runner = runner->next;

			pkt_t* detrop = pop_element_queue(list);

			if(list->size == 0 && pkt_get_length(packet) == 0 && pkt_get_seqnum(packet) == waited_seqnum-1){
				pkt_del(detrop);
				// finish
				return -10;
			}

			pkt_t* ack = create_ack(waited_seqnum, PTYPE_ACK, list->size);
			fprintf(stderr, "Sending ack %d normal\n", waited_seqnum);
			send_ack(ack, sfd, last_timestamp);
			pkt_del(ack);

			pkt_del(detrop); //detrop == packet
		}
		else
			return 0; // not the waited
	}
	return 0;
}

/*
 * Reads data from the sfd, and add the packets to the list
 * If the packet readed has a seqnum out of the window, an ack is directly sent with the
 * appropriate seqnum. If a NACK is received, we just ignore it. Else, we call write_in_sequence
 * after we added the packet to the queue at the right place with add_specific_queue
 * @list: the list
 * @window: a pointer to update the window size
 * @seqnum: the seqnum of the first packet waited
 * @return: -1 in case of error, -2 if the transfer is done, -1 otherwise
 */
int read_to_list_r(list_t* list, int sfd, int fd){
	if(!list){
		fprintf(stderr, "BIG ERROR: list NULL!\n");
		return -1;
	}
	char buffer[MAX_READ_SIZE];
	int readed = recv(sfd, buffer, MAX_READ_SIZE, 0);
	if(readed == -1){
		fprintf(stderr, "Error while receving data [read_to_list_r]\n");
		printf("%s\n", strerror(errno));
	}
	else{
		pkt_t* pkt = pkt_new();
		if(!pkt){
			fprintf(stderr, "Not enough memory while creation a packet [read_to_list_t]\n");
		}
		else{
			int err = pkt_decode(buffer, readed, pkt);
			if(err != 0){
				fprintf(stderr, "Wrong package, CRC\n");
				pkt_del(pkt);
				return -1;
			}
			if(pkt_get_length(pkt) > MAX_PAYLOAD_SIZE){
				fprintf(stderr, "DATA packet with length > MAX_PAYLOAD_SIZE\n");
				pkt_del(pkt);
				return -1;
			}
			if(pkt_get_tr(pkt) == 1){
				// packet troncated
				fprintf(stderr, "Packet was troncated\n");
				pkt_t* nack = create_ack(pkt_get_seqnum(pkt), PTYPE_NACK, list->size);
				if(!nack){
					fprintf(stderr, "Error while creating package, [rolr]\n");
					pkt_del(pkt);
					return -1;
				}
				send_ack(nack, sfd, last_timestamp);
				pkt_del(nack);
				pkt_del(pkt);
			}
			else if(check_in_window(pkt_get_seqnum(pkt))){ // dans la window
				add_specific_queue(list, pkt); // stock packet in list
				last_timestamp = pkt_get_timestamp(pkt);
				int not_in_sequence = write_in_sequence(list, sfd, fd); // writes to fd and sends ack
				if(not_in_sequence == -1){
					fprintf(stderr, "Error write in sequence\n");
					return -1;
				}
				if(not_in_sequence == -10){
					return -2;
				}
				else if(not_in_sequence == 1){
					//if the first packet needed is still not here, we send an ack with the waited one
					pkt_t* ack = create_ack(waited_seqnum, PTYPE_ACK, list->size);
					if(!ack){
						fprintf(stderr, "Error while creating package, [rolr]\n");
						return -1;
					}
					fprintf(stderr, "Sending ack %d wainted de first\n", waited_seqnum);
					send_ack(ack, sfd, last_timestamp);
					pkt_del(ack);
				}
			}
			else if(pkt_get_seqnum(pkt) == waited_seqnum-1 && pkt_get_length(pkt) == 0 && pkt_get_type(pkt) == PTYPE_DATA){
				pkt_t* ack = create_ack(waited_seqnum-1, PTYPE_DATA, list->size);
				if(!ack){
					fprintf(stderr, "Error while creating the last ack\n");
					pkt_del(pkt);
					return -1;
				}
				send_ack(ack, sfd, last_timestamp);
				pkt_del(ack);
				pkt_del(pkt);
				return -2;
			}
			else{
				// out of window
				fprintf(stderr, "Packet with seqnum %d was out of window, seqnum waited %d and window size %d\n", pkt_get_seqnum(pkt), waited_seqnum, window_size);
				pkt_del(pkt);
				pkt_t* ack = create_ack(waited_seqnum, PTYPE_ACK, list->size);
				if(!ack){
					fprintf(stderr, "Error while creating package, [rolr]\n");
					pkt_del(pkt);
					return -1;
				}
				send_ack(ack, sfd, last_timestamp);
				pkt_del(ack);

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

    struct sockaddr_in6 sock;
    socklen_t len = sizeof(sock);

    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(sfd, &fd);
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    int selected = select(sfd+1, &fd, NULL, NULL, &tv);
    if(selected == -1){
    	fprintf(stderr, "Error while the first connection of the socket\n");
    	return -1;
    }
    if(selected == 0){
    	fprintf(stderr, "No connection for the receiver. End\n");
    	return 1;
    }

    int nread = recvfrom(sfd, NULL, 0, MSG_PEEK, (struct sockaddr*) &sock, &len);
    if(nread == -1){
        fprintf(stderr, "Error using recvfrom\n");
        return -1;
    }

    int done = connect(sfd, (struct sockaddr*) &sock, sizeof(struct sockaddr_in6));
    if(done == -1){
        fprintf(stderr, "Error using connect");
        return -1;
    }
    printf("Connected!\n");

    return 0;
}

/*
 * Function that will do the major part of the receiver part
 * @sfd: the file descriptor of the socket
 * @fileOut: the file desctriptor where we will print the data
 * @return: -1 in case of error, 0 otherwize
 */
int process_receiver(int sfd, int fileOut){
	fd_set check_fd; // for the function select()
	int retval; // return value of select
	list_t* list = list_create();

	if(!list){
		fprintf(stderr, "Not enough memory to create the list. Fatal error.\n");
		exit(EXIT_FAILURE);
	}

	// check the maximum fd, may be fileOut ?
	int max_fd;
	if(sfd > fileOut)
		max_fd = sfd;
	else
		max_fd = fileOut;

	while(1){

		FD_ZERO(&check_fd);
		FD_SET(sfd, &check_fd);

		struct timeval tv;
		tv.tv_sec = 20;
		tv.tv_usec = 0;
		retval = select(max_fd+1, &check_fd, NULL, NULL, &tv);
		if(retval==0){
			fprintf(stderr, "EOF confirmed 1\n");
			break;
		}
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
			int err = read_to_list_r(list, sfd, fileOut);
			if(err == -2){
				fprintf(stderr, "EOF confirmed\n");
				break;
			}
		}
	}
	free(list);
	return 0;
}

/*
 * Main function. Handle the options and runs process_sender
 */
int main(int argc, char* argv[]){

	/* default values */
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
		fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
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
	int err_wait = wait_for_client(sfd);
	if(sfd > 0 && err_wait < 0){
		fprintf(stderr, "Error wait_for_client\n");
		if(fd != 1)
			close(fd);
		return -1;
	}
	else if(sfd > 0 && err_wait == 1){
		fprintf(stderr, "No connection confirmed. End\n");
		if(fd != 1)
			close(fd);
		return EXIT_FAILURE;
	}
	process_receiver(sfd, fd);

	close(sfd);

	if(fd!=1){
		close(fd);
	}

	return EXIT_SUCCESS;
}
