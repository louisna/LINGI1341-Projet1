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

#define MAX_READ_SIZE 528 
#define RETRANSMISSION_TIMER 2 


int seqnum = 0; // the waited seqnum
int window_size = 1; // the size of the window
int seqnum_EOF = 0; // indicates if the EOF has been reached
pkt_t* pkt_fin = NULL; // packet to indicate the end of transmission
struct timeval tv; // rt at the launch

/*
 * Function that will send a packet to the socket
 * @pkt: the adress of the packet to be sent
 * @sfd: the socket file descriptor
 * @return: -1 in case of error, 0 otherwise
 */
int send_packet(pkt_t* pkt, int sfd){
	time_t current_time = time(NULL);
	uint32_t  a_lo = (uint32_t) current_time;

	pkt_status_code err1 = pkt_set_timestamp(pkt, a_lo);
	if(err1!=PKT_OK){
		fprintf(stderr, "Error while encoding time in packet : error is %d\n",err1);
		return -1;
	}

	size_t length = 3*sizeof(uint32_t);
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
	fprintf(stderr, "Sending the packet with seqnum %d\n", pkt_get_seqnum(pkt));
	ssize_t err3 = send(sfd, buff, length, 0);
	if(err3==-1){
		fprintf(stderr, "Error while sending packet\n");
		return -1;
	}
	return 0;
}

/*
 * Checks and delete all the packets with a seqnum below seqnum_ack
 * @list: the linked_list with the packets sended, != NULL
 * @seqnum_ack: the seqnum of the ack
 * @return: -1 in case of error, 1 if EOF ack, 0 otherwise
 */
int packet_checked(list_t* list, int seqnum_ack){
    node_t* runner = list->head;
    pkt_t* packet ;
    int seqn;

    while(runner != NULL){

        packet = runner->packet;
    	seqn = pkt_get_seqnum(packet);
    	// to handle the case 255->0 for the seqnum
    	// here we use MAX_WINDOW_SIZE and not window_size because if we get a NACK, the window is divided by 2
    	// but the ACKed packets should still be removes (it does not affect the receiver)
    	// it's faster like this and does not hurt the receiver
    	if(!(255+seqnum_ack-seqn <= MAX_WINDOW_SIZE || (seqnum_ack-seqn > 0 && seqnum_ack-seqn <= MAX_WINDOW_SIZE))){
    		return 0;
    	}
    	if(list->size == 1 && pkt_get_length(packet) == 0 && seqnum_EOF == 1){
    		// EOF reached
    		return 1;
    	}

        runner = runner->next;

        pkt_t* packet_pop = pop_element_queue(list);
        if(packet_pop == NULL){
            fprintf(stderr, "List was in fact empty [packet_checked]\n");
            return -1;
        }
		fprintf(stderr, "Packet seqnum %d was ack, size of list %d\n", pkt_get_seqnum(packet_pop), list->size);
        pkt_del(packet_pop);
    }
    return 0;

}

/*
 * Check the ack-packet receved from sfd, and delete from list all
 * the packets with a seqnum below the seqnum of the ack
 * @sfd: the socket file descriptor
 * @list: the linked-list with all the packets
 * @last_seqnum: the seqnum of the last element of the window+1
 * @window: a pointer to the actual window size, may change with
 * @return: -1 in case of error, 1 if the ack of the EOF paquet was received, 0 otherwise
 */
int check_ack(int sfd, list_t* list){
    if(list == NULL){
        fprintf(stderr, "List NULL [check_ack]\n");
        return -1;
    }

    char buffer[MAX_READ_SIZE];
    int readed = recv(sfd, buffer, MAX_READ_SIZE, 0);
    if(readed == -1){
        fprintf(stderr, "Error while receving ack [check_ack]\n");
        return -1;
    }
    else{
        pkt_t* pkt = pkt_new();
        if(!pkt){
            fprintf(stderr, "Not enough memory while creating a new pkt [check_ack]\n");
        }
        else{
            int err = pkt_decode(buffer, readed, pkt);
            if(err){
            	fprintf(stderr, "Impossible to decode te package [check_ack]\n");
            	pkt_del(pkt);
            	return -1;
            }
            if(pkt_get_tr(pkt) == 1){
            	fprintf(stderr, "ACK/NACK with tr = 1, discarded\n");
            	pkt_del(pkt);
            	return -1;
            }
            // we get here the new window_size
            window_size = pkt_get_window(pkt);
            fprintf(stderr, "Received ack of %d. Window of the receiver in now of %d\n", pkt_get_seqnum(pkt), window_size);
            int seqnum_ack = pkt_get_seqnum(pkt);

            if(pkt_get_type(pkt) == PTYPE_ACK){
                // we can delete the packets from list with the accumilative ack
                err = packet_checked(list, seqnum_ack);
                if(err == -1){
                	fprintf(stderr, "Error in packet_checked\n");
                }
                else if(err == 1){
                	fprintf(stderr, "EOF reached\n");
                	pkt_del(pkt);
                	return 1;
                }
            }
            else if(pkt_get_type(pkt) == PTYPE_NACK){
            	fprintf(stderr, "NACK of seqnum %d received\n", pkt_get_seqnum(pkt));
            }
            else{
                fprintf(stderr, "Wrong type of ack packet [check_ack]\n");
                pkt_del(pkt);
                return -1;
            }

            pkt_del(pkt);
            return 0;
        }
    }
    return 0;
}

/*
 * Checks if the seqnum from the packet received is in the window
 * of the receiver
 * @return: 1 if seqnum is in the window, 0 otherwise
 */
int check_in_window(int seqnum_check, int first){
	if(seqnum_check >= first){ // no 255 -> 0
		if(seqnum_check - first <= window_size - 1)
			return 1;
		else
			return 0;
	}
	else{// 255->0
		if(255 + seqnum_check - first <= window_size - 1)
			return 1;
		else
			return 0;
	}
}

/*
 * Checks all the elements of the list (window) and sends all the packets
 * with timestamp - actual_time >= RETRANSMISSION TIMER
 * If the last element to be acknoledged is the paket to indicate the EOF
 * and this packet is on timeout, we assume that the ack has been lost
 * and the receiver is already shut down.
 * @list: the list
 * @sfd: the socket file descriptor
 * @return: 1 if the packet with EOF is the only remaining packet in the list
 * 			to be acknoledged. 0 otherwise
 */
int check_timeout(list_t* list, int sfd){
	// get the time and keep the 32 low bits
	time_t current_time = time(NULL);
	uint32_t  a_lo = (uint32_t) current_time;

	node_t* runner = list->head;
	while(runner != NULL){
		pkt_t* packet = runner->packet;
		if(!check_in_window(pkt_get_seqnum(packet), pkt_get_seqnum(list->head->packet))){
			// we only send the timeout packets which fit the window (in case of NACK received before)
			return 0;
		}
		uint32_t time_sent = pkt_get_timestamp(packet);
		if(a_lo - time_sent >= RETRANSMISSION_TIMER){
			fprintf(stderr, "Timeout of %d\n", pkt_get_seqnum(packet));
			if(pkt_get_length(packet) == 0 && seqnum_EOF == 1 && list->size == 1){
				pkt_del(pkt_fin);
				return 1;
			}
			int err = send_packet(packet, sfd);
			if(err){
				fprintf(stderr, "Impossible to send again the packet [check_timeout]\n");
			}
		}
		runner = runner->next;
	}
	return 0;
}



/*
 * Reads the data on the fd and creates a paquet for each of the data read
 * until the size of the list is equal to the size of the window
 * @fd: the file descriptor where we read the data
 * @list: the linked_list, != NULL
 * @window: thesize of the window
 * @new_seqnum: the number of the first seqnum for the new packet
 *				should be +1 of the last seqnum of an existing packet
 * @return: the number of the new new_seqnum (same interpretation)
 * 			or -2 if EOF reached
 */
void read_to_list(int fd, list_t* list, int sfd){
	if(!list){
		fprintf(stderr, "BIG ERROR: list NULL!\n");
		return;
	}
	if(list->size >= window_size){
		// the list is full
		return;
	}
		char payload[MAX_PAYLOAD_SIZE];
		int readed = read(fd, payload, MAX_PAYLOAD_SIZE);
		if(readed == -1){
			fprintf(stderr, "Impossible to read data on fileIn [read_to_list]\n");
		}
		else{
			pkt_t* pkt = pkt_new();
			if(!pkt){
				fprintf(stderr, "Impossible to create the pkt [read_to_list]\n");
			}
			else{
				int err1 = 0;
				int err2 = pkt_set_type(pkt, PTYPE_DATA);
				int err3 = pkt_set_tr(pkt, 0);
				int err4 = pkt_set_window(pkt, 0);
				int err5 = 0;
				err1 = pkt_set_seqnum(pkt, seqnum);
				err5 = pkt_set_payload(pkt, payload, readed);

				// If it is a packet with EOF, we don't send it, but we update the pkt_fin
				if(readed == 0){
					seqnum_EOF = 1;
					err1 = pkt_set_seqnum(pkt, seqnum);
					pkt_fin = pkt;
				}
				// if an error occured during the initialisation of the packet
				if(err1 || err2 || err3 || err4 || err5){
					fprintf(stderr, "Error while seting the pkt\n");
					pkt_del(pkt);
					return;
				}

				fprintf(stderr, "New packet seqnum %d was added, list size = %d\n", pkt_get_seqnum(pkt), list->size+1);

				// We send the packet if and only if it is not a packet with EOF
				if(readed != 0){
					int err6 = send_packet(pkt,sfd);
					if(err6){
						fprintf(stderr, "Error while sending the packet for the first time\n");
					}

					// Add it to the window
					int err = add_element_queue(list, pkt);
					if(err){
						fprintf(stderr, "Error while adding the packet to the queue, discarded\n");
					}
					//new seqnum
					seqnum = (seqnum + 1)%256;
				}
			}
		}
}

/*
 * Sends the last packet only and only if the list is empty.
 * @pre: seqnum_EOF == 1, the EOF was reached when reading
 * @list: the list
 * @sfd: the socket file descriptor
 * @return: 1 if the list was not empty, -1 in case of error, 0 otherwise
 */
int final_send(list_t* list, int sfd){
	if(list->size == 0){
		// all data packets have been sent and received
		// just send the final packet
		int err = send_packet(pkt_fin, sfd);
		if(err){
			fprintf(stderr, "Final packet could not be sent\n");
			return -1;
		}
		pkt_del(pkt_fin);
		return 0;
	}
	return 1;
}

/*
 * Function that will do the major part of the sender part
 * @sfd: the file descriptor of the socket
 * @fileIn: the file desctriptor from where we take data
 * @return: -1 in case of error, 0 otherwize
 */
int process_sender(int sfd, int fileIn){
	fd_set check_fd; // for the function select()
	int retval; // return value of select
	list_t* list = list_create();

	tv.tv_sec = RETRANSMISSION_TIMER;
	tv.tv_usec = 0;

	int max_fd = sfd > fileIn ? sfd : fileIn;

	while(1){

		FD_ZERO(&check_fd);
		FD_SET(sfd, &check_fd);
		FD_SET(fileIn, &check_fd);

		retval = select(max_fd+1, &check_fd, NULL, NULL, &tv);
		tv.tv_sec = RETRANSMISSION_TIMER;
		tv.tv_usec = 0;

		if(retval == -1){
			fprintf(stderr, "Error from select [process_sender]");
			free(list);
			return -1;
		}

		else if(FD_ISSET(sfd, &check_fd)){
			int retour = check_ack(sfd, list);
			if(retour == 1){
				fprintf(stderr, "EOF confirmed. End\n");
				break;
			}
		}
		else if(FD_ISSET(fileIn, &check_fd) && seqnum_EOF == 0){
			read_to_list(fileIn, list, sfd);
		}
		/* We check if the EOF was read */
		if(seqnum_EOF == 1){
			int fin = final_send(list, sfd);
			if(fin == 0){
				fprintf(stderr, "End from read_to_list\n");
				break;
			}
		}
		/* We send all the packets with timeout */
		int time_out = check_timeout(list,sfd);
		if(time_out == 1){
			fprintf(stderr, "Ack useless at this point. End\n");
			break;
		}



	}
	free(list);
	// End of the process
	return 0;
}

/*
 * Debug function to print the data associated with all the packets in the window
 */
void print_list(list_t* list){
	node_t* runner = list->head;
	while(runner != NULL){
		print_data(runner->packet);
		runner = runner->next;
	}
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
			"- hostname: domain name or IPv6 receiver's adress\n"
			"- port: UDP port number where the sender was plugged\n");
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
			"- hostname: domain name or IPv6 receiver's adress\n"
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
			fd = STDIN_FILENO;
		}
	}
	else{
		fd = STDIN_FILENO;
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
	sfd = create_socket(NULL, -1, &addr, port); /* Connected */
	if(sfd < 0){
		fprintf(stderr, "Failed to create the socket\n");
		exit(EXIT_FAILURE);
	}

	process_sender(sfd, fd);

	if(fd != 0)
		close(fd);

	return EXIT_SUCCESS;
}
