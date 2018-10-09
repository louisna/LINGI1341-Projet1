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

void read_write_loop(int sfd){

    fd_set reader;

    int c = 1;
    int retval;

    char buffer[1024];

    while(1){
        FD_ZERO(&reader);
        FD_SET(sfd, &reader);
        FD_SET(STDIN_FILENO, &reader);

        retval = select(sfd+1, &reader, NULL, NULL, 0);
        if(retval == -1){
            fprintf(stderr, "Error select\n");
        }

        else if(FD_ISSET(STDIN_FILENO, &reader)){
            c = read(STDIN_FILENO, buffer, sizeof(buffer));

            if(c < 0){
                fprintf(stderr, "Error writing 2\n");
                return;
            }

            size_t readed = (size_t) c;

            int error = write(sfd, buffer, readed);
            if(error == -1){
                fprintf(stderr, "Error while writing in sfd\n");
            }
        }
        else if(FD_ISSET(sfd, &reader)){
            c = read(sfd, buffer, sizeof(buffer));
            if(c < 0){
                fprintf(stderr, "Error reading\n");
                return;
            }

            size_t readed = (size_t) c;

            int error = write(STDOUT_FILENO, buffer, readed);

            if(error == -1){
                fprintf(stderr, "Error while writing in stdout\n");
                return;
            }
        }
    }
}