CC = gcc
CFLAGS = -Wall -g


all: client server

client: client.o
	$(CC) $(CFLAGS) -o client client.o

server: server.o
	$(CC) $(CFLAGS) -o server server.o

client.o: secure_client.c
	$(CC) $(CFLAGS) -c secure_client.c -o client.o

server.o: secure_server.c
	$(CC) $(CFLAGS) -c secure_server.c -o server.o

clean:
	rm -f client client.o server server.o
