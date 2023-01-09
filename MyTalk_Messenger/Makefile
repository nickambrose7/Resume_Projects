CC = gcc
CFLAGS =  -Wall -ansi -pedantic -g -c -I
LDFLAGS = -Wall -pedantic -g -L


all: mytalk

mytalk: mytalk.o
	$(CC) -o mytalk $(LDFLAGS) include mytalk.o -ltalk -lncurses

mytalk.o: mytalk.c
	$(CC) -o mytalk.o $(CFLAGS) include mytalk.c

clean:
	rm *.o 
	
target: dependency
	action