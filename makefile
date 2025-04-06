all: src/server.c
	gcc -o server src/server.c -Wall -lpthread
clean:
	rm server