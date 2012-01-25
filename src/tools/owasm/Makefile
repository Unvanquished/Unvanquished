# yeah, couldn't do more simple really

CC=gcc
CFLAGS=-g -Wall

default:	owasm

owasm:	owasm.c cmdlib.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f owasm *~ *.o

