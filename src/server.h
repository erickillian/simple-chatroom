#ifndef SERVER_H
#define SERVER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define DEFAULT_PORT 8080
#define DEFAULT_HOSTNAME "localhost"

#define MAX_CLIENTS 3
#define MAX_USERNAME_LENGTH 50
#define MAX_ROOMNAME_LENGTH 50
#define MAX_MESSAGE_LENGTH 1024

typedef struct {
    int socket;
    struct sockaddr_in address;
    bool is_connected;
    bool is_in_room;
    char username[MAX_USERNAME_LENGTH];
    char roomname[MAX_ROOMNAME_LENGTH];
} connection_t;

void sigint_handler(int signum);

void *handle_client(void *arg);

#endif