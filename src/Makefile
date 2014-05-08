# make for select server

CC=gcc
CFLAGS=-Wall -ggdb -lpthread

ex: llist.o server.o client.o main.o port_fd
	
port_fd:
	$(CC) $(CFLAGS) llist.o main.o client.o server.o -o port_fd
tcp_clnt: 
	 $(CC) $(CFLAGS) -o tcp_clnt tcp_clnt.c
main.o: main.c
	$(CC) $(CFLAGS) -O -c main.c
llist.o: llist.c
	$(CC) $(CFLAGS) -O -c llist.c
server,o: server.c
	$(CC) $(CFLAGS) -O -c server.c
client.o: client.c
	$(CC) $(CFLAGS) -O -c client.c
clean:
	rm -f *.o *.bak port_fd
clean_bak:
	rm -f *.o *.bak
