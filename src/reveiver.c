#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "envoyer_recevoir_donnees/real_address.h"
#include "envoyer_recevoir_donnees/create_socket.h"
#include "envoyer_recevoir_donnees/read_write_loop.h"
#include "envoyer_recevoir_donnees/wait_for_client.h"

int main(int argc, char* argv[]){

	/* default values */
	int client = 0;
	int port = 12345;
	int opt;
	char* host = "::1";
	char* file = NULL;
	int has_file = 0;

	if(argc < 3){
		fprintf(stderr, "The program needs at least 2 arguments to work:\n"
			"- hostname: domain name or IPv6 sender's adress\n"
			"- port: UDP port number where the receiver was plugged\n");
		exit(EXIT_FAILURE);
	}

	while((opt = getopt(argc, argv, "f:")) != -1){
		switch(opt){
			case 'f':
				has_file = 1;
				file = optarg;
				break;
			default:
				fprintf(stderr, "The only accepted option is -f X\n"
					"where X is the name of an existing file\n");
				break;
		}
	}

	if(has_file && argc < 5){
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

	if(sfd > 0 && wait_for_client(sfd) < 0){
		fprintf(stderr, "Could not connect the socket after the first message\n");
		close(sfd);
		exit(EXIT_FAILURE);
	}

	// do something

	close(sfd);

	return EXIT_SUCCESS;
}