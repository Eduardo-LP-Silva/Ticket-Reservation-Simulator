#Makefile

server: server.h server.c
	gcc server.c server.h -o server -Wall -lrt -lpthread
	
client: client.c
	gcc client.c -o client -Wall -lrt -lpthread

clean:
	rm -f client
	rm -f server