#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include "nyancat.h"
#include "packet_implement.h"


int read_to_list(int fd, list_t* list, int window, int new_seqnum, int sfd){
	return 0;
}

int process_receiver(int sfd, int fileOut){
	fd_set check_fd; // for the function select()
	int retval; // return value of select
	list_t* list = list_create();
	int seqnum = 0;

	// check the maximum fd, may be fileOut ?
	int max_fd = sfd > fileOut ? sfd : fileOut;

	while(1){

		FD_ZERO(&check_fd);
		FD_SET(sfd, &check_fd);

		retval = select(max_fd+1, &check_fd, NULL, NULL, 0);

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
		}
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
			d = STDOUT_FILENO;
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

	close(sfd);

	return EXIT_SUCCESS;
}
