CC = gcc
CFLAGS =  -Wall -ansi -pedantic -g
LDFLAGS = -Wall -pedantic -g


all: hencode hdecode

hdecode: hdecode.o huffstuff.o 
	$(CC) -o hdecode $(LDFLAGS) hdecode.o huffstuff.o

hencode: hencode.o huffstuff.o
	$(CC) -o hencode $(LDFLAGS) hencode.o huffstuff.o

hecode.o: hencode.c huffmancoding.h 
	$(CC) $(CFLAGS) -c hencode.c

hdecode.o: hdecode.c huffmancoding.h huffstuff.c
	$(CC) -c $(CFLAGS) hdecode.c 

huff.o: huffstuff.c huffmancoding.h
	$(CC) $(CFLAGS) -c huffstuff.c	

clean:
	rm *.o 

test: hencode 
	./$(MAIN) input output

target: dependency
	action