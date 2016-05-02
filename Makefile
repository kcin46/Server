CC = gcc
PROGS = server

server: server.o
	gcc -Wall -o server server.o

server.o: server.c common.h
	$(CC) -Wall -c server.c
