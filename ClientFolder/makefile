#makefile for server
Client: Client.o token.o
	gcc Client.o token.o -o Client.exe
Client.o: Client.c
	gcc -c Client.c -o Client.o
token.o: token.c token.h
	gcc -c token.c -o token.o
clean:
	rm -f *.o myftp