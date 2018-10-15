#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>        /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include "nyancat.h"
#include "packet_implement.h"
#define MAX_READ_SIZE 1024 // need to be changed ?



/*
 * Reads the data on the fd and creates a paquet for each of the data read
 * until the size of the list is equal to the size of the window
 * @fd: the file descriptor where we read the data
 * @list: the linked_list, != NULL
 * @window: thesize of the window
 * @new_seqnum: the number of the first seqnum for the new packet
 *				should be +1 of the last seqnum of an existing packet
 * @return: the number of the new new_seqnum (same interpretation)
 */

// !!!!!!!!!!!!!!!!!!!! peut-etre changer new_seqnum par le dernier numero de sequence deja utilise ?
// !!!!!!!!!!!!!!!!!!!! et retourner aussi le dernier deja utilise ? Peut-etre plus simple pour apres
int read_to_list(int fd, list_t* list, int window, int new_seqnum){
	if(!list){
		fprintf(stderr, "BIG ERROR: list NULL!\n");
		return -1;
	}
	while(list->size < window){
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
				int err1 = pkt_set_seqnum(pkt, new_seqnum);
				int err2 = pkt_set_type(pkt, PTYPE_DATA);
				int err3 = pkt_set_tr(pkt, 0);
				int err4 = pkt_set_window(pkt, window);
				int err5 = pkt_set_payload(pkt, payload, readed);
				if(err1 || err2 || err3 || err4 || err5){
					fprintf(stderr, "Error while seting the pkt\n");
				}
				int err = add_element_queue(list, pkt);
				if(err){
					fprintf(stderr, "Error while adding the packet to the queue, discarded\n");
				}
				new_seqnum = (new_seqnum + 1)%256; // 256 ?
			}	
		}
	}
	return new_seqnum;
}

/*
 * Function that will do the major part of the sender part
 * @sfd: the file descriptor of the socket
 * @fileIn: the file desctriptor from where we take data
 * @return: -1 in case of error, 0 otherwize
 */
int process(int sfd, int fileIn){
	fd_set check_fd; // for the function select()
	int retval; // return value of select
	int readed; // count the number of bytes read by read/receve

	char buffer[MAX_READ_SIZE];

	// check the maximum fd, may be fileIn ?
	int max_fd = sfd > fileIn ? sfd : fileIn;

	while(1){

		/*
		Quand faisons nous la verification des segments deja envoyes?
		C'est-a-dire leur ACK pour voir si on peut les retirer de la liste

		A quel moment decidonc-nous d'envoyer les packets deja dans la liste ?
		Ici (avant le select) ou apres tout ? ou dans les else if ?
		*/

		FD_ZERO(&check_fd);
		FD_SET(sfd, &check_fd);
		FD_SET(fileIn, &check_fd);

		retval = select(max_fd+1, &check_fd, NULL, NULL, 0); // add time for timeout
		if(retval == -1){
			fprintf(stderr, "Error from select");
			return -1; // vraiment ?
		}

		else if(FD_ISSET(fileIn, &check_fd)){
			// Read from fileIn, create packet, 
		}
		else if(FD_ISSET(sfd, &check_fd)){

		}
	}
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

	// do something
	int seqnum = 254;
	list_t* list = list_create();
	int error = read_to_list(fd, list, 3, seqnum);
	print_list(list);



	close(sfd);

	return EXIT_SUCCESS;
}