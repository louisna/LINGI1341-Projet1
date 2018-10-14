# Makefile for Project 1 for the course LINGI1341 Computer Network
# Hadrien Plancq & Louis Navarre
# Partially taken from the Makefile of the exercice "envoyer et recevoir des données"

# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

LDFLAGS = -rdynamic -lz

all: sender receiver

create_socket.o : src/envoyer_recevoir_donnees/create_socket.c src/envoyer_recevoir_donnees/create_socket.h
		gcc -c src/envoyer_recevoir_donnees/create_socket.c src/envoyer_recevoir_donnees/create_socket.h -std=c99 -lz

real_address.o: src/envoyer_recevoir_donnees/real_address.c src/envoyer_recevoir_donnees/real_address.h
	gcc -c src/envoyer_recevoir_donnees/real_address.c src/envoyer_recevoir_donnees/real_address.h -std=c99 -lz
#src/receiver.h
receiver : src/receiver.c create_socket.o real_address.o
	gcc -o receiver src/receiver.c create_socket.o real_address.o -std=c99 -lz

sender: src/sender.c create_socket.o real_address.o
	gcc -o sender src/sender.c create_socket.o real_address.o -std=c99 -lz
clean : rm -f sender receiver real_address.o create_socket.o