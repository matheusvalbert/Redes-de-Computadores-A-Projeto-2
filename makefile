all: p2p

p2p: client server

client: client.c
	gcc client.c -o client -pthread

server: server.c
	gcc server.c -o server -pthread