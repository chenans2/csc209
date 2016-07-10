PORT=53692
CFLAGS= -DPORT=\$(PORT) -g -Wall -std=c99 -Werror

friends_server: friends_server.o friends.o 
	gcc $(CFLAGS) -o friends_server friends_server.o friends.o

friends_server.o: friends_server.c friends.h
	gcc $(CFLAGS) -c friends_server.c

friends.o: friends.c friends.h
	gcc $(CFLAGS) -c friends.c

clean: 
	rm friends_server *.o
