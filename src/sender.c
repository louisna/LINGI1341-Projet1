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
#define RETRANSMISSION_TIMER 2 // pour l'instant
//#define MAX_WINDOW_SIZE 31;


int seqnum = 0; // the waited seqnum
int window_size = 5; // the size of the window


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

	ssize_t err3 = send( sfd, buff, length,0);
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
 * @return: -1 in case of error, 0 otherwise
 */
int packet_checked(list_t* list, int seqnum_ack){
    node_t* runner = list->head;
    pkt_t* packet ;
    int seqn;

    while(runner != NULL){

        packet = runner->packet;
    	seqn = pkt_get_seqnum(packet);
    	if(seqn>seqnum_ack){
    		return 0;
    	}

        runner = runner->next;

        pkt_t* packet_pop = NULL;
        int err = pop_element_queue(list, packet_pop);
        if(err){
            fprintf(stderr, "List was in fact empty [packet_checked]\n");
            return -1;
        }
        pkt_del(packet_pop);
    }
    return 0;

}

void send_specific_pkt(int sfd, list_t* list, int seqn){
	node_t* runner = list->head;
	while(runner != NULL){
		pkt_t* packet = runner->packet;
		if(seqn == pkt_get_seqnum(packet)){
			int err = send_packet(packet, sfd);
			if(err){
				fprintf(stderr, "Packet with NACK cannot be sent\n");
			}
		}
	}
	fprintf(stderr, "[send_specific_pkt] Cannot find the packet\n");
}

/*
 * Check the ack-packet receved from sfd, and delete from list all
 * the packets with a seqnum below the seqnum of the ack
 * @sfd: the socket file descriptor
 * @list: the linked-list with all the packets
 * @last_seqnum: the seqnum of the last element of the window+1
 * @window: a pointer to the actual window size, may change with 
 */
int check_ack(int sfd, list_t* list){
    if(list == NULL){
        fprintf(stderr, "List NULL [chack_ack]\n");
        return -1;
    }
    if(!list->size){
        fprintf(stderr, "List empty [check_ack]\n");
        return -1;
    }
    pkt_t* pkt_head = list->head->packet;
    int first_seqnum = pkt_get_seqnum(pkt_head);// premier seqnum qui n'a pas été ack

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
            	fprintf(stderr, "Impossible to decode teh package [check_ack]\n");
            	return -1;
            }
            window_size = pkt_get_window(pkt);
            int seqnum_ack = pkt_get_seqnum(pkt);


            if(pkt_get_type(pkt) == PTYPE_ACK){
                // we can delete the packets from list with the accumilative ack
                err = packet_checked(list, seqnum_ack);
                if(err){
                	fprintf(stderr, "Error in packet_checked\n");
                }
            }
            else if(pkt_get_type(pkt) == PTYPE_NACK){ // devoir juste retirer l'element specifique ?
                
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

int check_timeout(list_t* list, int sfd, int window){
	time_t current_time = time(NULL);	
	uint32_t  a_lo = (uint32_t) current_time;
	int count = 0;

	node_t* runner = list->head;
	while(runner != NULL && count < window){
		pkt_t* packet = runner->packet;
		uint32_t time_sent = pkt_get_timestamp(packet);
		if(a_lo - time_sent >= RETRANSMISSION_TIMER){
			int err = send_packet(packet, sfd);
			count++;
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

// !!!!!!!!!!!!!!!!!!!! peut-etre changer new_seqnum par le dernier numero de sequence deja utilise ?
// !!!!!!!!!!!!!!!!!!!! et retourner aussi le dernier deja utilise ? Peut-etre plus simple pour apres
void read_to_list(int fd, list_t* list, int sfd){
	if(!list){
		fprintf(stderr, "BIG ERROR: list NULL!\n");
		return;
	}
		char payload[MAX_PAYLOAD_SIZE]; // maybe put it before the wile loop
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
				int err4 = pkt_set_window(pkt, MAX_WINDOW_SIZE - list->size - 1);
				int err5 = 0;
				if(readed > 0){
					err1 = pkt_set_seqnum(pkt, seqnum);
					err5 = pkt_set_payload(pkt, payload, readed);
				}
				//else supprimé à check, on devrait pas renvoyer le seqnum -1 simplement ?
				if(err1 || err2 || err3 || err4 || err5){
					fprintf(stderr, "Error while seting the pkt\n");
				}

				//On envoie le packet
				
				int err6 = send_packet(pkt,sfd);
				if(err6){
					fprintf(stderr, "Error while sending the packet for the first time\n");
				}
				

				//On l'ajoute à la window
				int err = add_element_queue(list, pkt);
				if(err){
					fprintf(stderr, "Error while adding the packet to the queue, discarded\n");
				}
				seqnum = (seqnum + 1)%256; // 256 ?
			}	
		}
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

	// check the maximum fd, may be fileIn ?
	int max_fd = sfd > fileIn ? sfd : fileIn;

	while(1){
		FD_ZERO(&check_fd);
		FD_SET(sfd, &check_fd);
		FD_SET(fileIn, &check_fd);

		retval = select(max_fd+1, &check_fd, NULL, NULL, 0);

		if(retval == -1){
			fprintf(stderr, "Error from select [process_sender]");
			return -1; // vraiment ?
		}

		else if(FD_ISSET(sfd, &check_fd)){
			check_ack(sfd, list);
		}
		else if(FD_ISSET(fileIn, &check_fd)){
			//seqnum = read_to_list(fileIn, list, int window, seqnum, sfd ){
			// Read from fileIn, create packet,
			// reprendre le time
			//pas oublier de stopper le renvoi de timeout à la fin de la window size
			read_to_list(fileIn, list, sfd);
		}
		//check timeout
		

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
	/*

	char* a = "Salut";
	pkt_t* pkt = pkt_new();
	pkt_set_payload(pkt, a, 5);
	send_packet(pkt, sfd);
	*/
	

	// tests

	// do something
	process_sender(sfd, fd);


	close(sfd);

	return EXIT_SUCCESS;
}