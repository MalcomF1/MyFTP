CC = gcc
CFLAGS = -Wall

all: server

server: server.o func.o command.o
	$(CC) $(CFLAGS) -o server server.o func.o command.o

server.o: server.c command.h func.h
	$(CC) $(CFLAGS) -c server.c

func.o: func.c func.h
	$(CC) $(CFLAGS) -c func.c

command.o: command.c command.h
	$(CC) $(CFLAGS) -c command.c

clean:
	rm -f server *.o