EXECS = client server
OBJS = client.o server.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

all: client server
	mkdir -p build
	mkdir -p build/objects
	mv client.o build/objects/client.o
	mv server.o build/objects/server.o

client : client.o
	$(CC) client.o -o client

server : server.o
	$(CC) server.o -o server

client.o : CS450Header.h client.cpp
	$(CC) $(CFLAGS) client.cpp

server.o : CS450Header.h server.cpp
	$(CC) $(CFLAGS) server.cpp


clean:
	rm -rf build $(EXECS)