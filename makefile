all: hello

hello: server.o
	gcc server.o -pthread -lm -o server
	gcc client.o -pthread -o client

server.o: server.c
	gcc -c -std=c11 server.c
	gcc -c -std=c11 client.c

clean:
	rm *.o server client
