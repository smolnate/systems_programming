all: server client

client:
	gcc -lssl -lcrypto -g -o WTF WTFClient.c

server:
	gcc -g -pthread -o WTFserver WTFServer.c WTFserver.h

test:
	gcc -pthread -o WTFtest WTFtest.c

clean: 
	rm -f WTF WTFserver WTFtest