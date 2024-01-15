CC = gcc
CFLAGS = -Wall

all: client

client: client.o command.o
	$(CC) $(CFLAGS) -o client client.o command.o

client.o: client.c command.h
	$(CC) $(CFLAGS) -c client.c

command.o: command.c command.h
	$(CC) $(CFLAGS) -c command.c

clean:
	rm -f client *.o