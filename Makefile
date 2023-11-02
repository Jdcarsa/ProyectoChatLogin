all: server.o	client.o
	gcc -o server server.o -lpthread
	gcc -o client client.o -lpthread

server.o: server.c
	gcc -c -o server.o server.c

client.o: client.c
	gcc -c -o client.o client.c 

doc:
	doxygen

clean:
	rm -rf *.o server client docs