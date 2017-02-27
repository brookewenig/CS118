CC=gcc
CFLAGS=-g
DEPS = # header file 

all: server client

server: server.o
	$(CC) -o server server.o $(CFLAGS)

client: client.o
	$(CC) -o client client.o $(CFLAGS)

server.o: server.c

client.o: client.c

clean:
	rm -f *.o
