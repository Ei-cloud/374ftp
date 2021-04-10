#makefile for server
myftpd:
	gcc Server.c -o Server.exe
myftp: myftpd.c
	gcc -Client.c -o Client.exe
clean:
	rm -f *.o myftpd