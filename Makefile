# Makefile for Project 1 for the course LINGI1341 Computer Network
# Hadrien Plancq & Louis Navarre
# Partially taken from the Makefile of the exercice "envoyer et recevoir des données"

# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
#CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

LDFLAGS = -rdynamic -lz

all: sender receiver

nyancat.o : src/nyancat.c src/nyancat.h
		gcc -c src/nyancat.c src/nyancat.h $(CFLAGS) -lz

packet_implement.o: src/packet_implement.c src/packet_implement.h
	gcc -c src/packet_implement.c src/packet_implement.h $(CFLAGS) -lz

receiver : src/receiver.c nyancat.o packet_implement.o
	gcc -o receiver src/receiver.c nyancat.o packet_implement.o $(CFLAGS) -lz

sender: src/sender.c nyancat.o packet_implement.o
	gcc -o sender src/sender.c nyancat.o packet_implement.o $(CFLAGS) -lz

test_packet.o: ./tests/test_packet.c ./src/packet_implement.h
	gcc -c ./tests/test_packet.c $(CFLAGS) -lcunit

test_nyancat.o: ./tests/test_nyancat.c ./src/packet_implement.h ./src/nyancat.h
	gcc -c ./tests/test_nyancat.c $(CFLAGS) -lcunit

test_packet: test_packet.o
	gcc -o test_packet test_packet.o  $(CFLAGS) -lcunit
test_nyancat: test_nyancat.o
	gcc -o test_nyancat test_nyancat.o $(CFLAGS) -lcunit

test : test_packet test_nyancat
	./test_packet 
	./test_nyancat

clean : rm -f sender receiver nyancat.o packet_implement.o