CC = gcc
CFLAGS =  -Wall -pedantic -g -c -I
LDFLAGS = -Wall -pedantic -g 

all: mush2

mush2: mush2.o
	$(CC) -o mush2 $(LDFLAGS) mush2.o libmush.dylib

mush2.o: mush2.c
	$(CC) -o mush2.o $(CFLAGS) include mush2.c

clean:
	rm *.o 
	
target: dependency
	action