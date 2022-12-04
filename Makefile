all: init server client clean

init: 
	mkdir -p ./executables

server: server.o
	gcc -o ./executables/server server.o

client: client.o
	gcc -o ./executables/client client.o

server.o: ./src/Serveur/server.c ./src/Serveur/server.h ./src/Serveur/client.h ./src/Serveur/group.h
	gcc -c ./src/Serveur/server.c 

client.o: ./src/Client/client.c  ./src/Client/client.h
	gcc -c ./src/Client/client.c

clean:
	rm -f *.o