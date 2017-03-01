CC = gcc

default: server par obs

server: prog3_server.c
	$(CC) -o server prog3_server.c

par: prog3_participant.c
	$(CC) -o par prog3_participant.c

obs: prog3_observer.c
	$(CC) -o obs prog3_observer.c

# server.o: prog3_server.c
# 	$(CC) -c prog3_server.c