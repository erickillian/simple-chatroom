#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#include <pthread.h>

#include <signal.h>

#define MAX_CLIENTS 1000
#define BUFSIZE 1048
#define MAXLINE 20000

/* Client structure */
typedef struct
{
    int socket;
    char *name;
    char *room;
} client;

client *clients[MAX_CLIENTS];

int num_clients;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_connection(void *p_client_socket);
void client_add(client *c);
void client_remove(client *c);
void send_message(char *message, client *cli);
void free_clients();
void list_clients();

/* Print ip address */
void print_client_addr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* replace '\n' and '\r' with '\0' */
void strip_newline(char *s)
{
    while (*s != '\0')
    {
        if (*s == '\r' || *s == '\n')
        {
            *s = '\0';
        }
        s++;
    }
}

void *handle_connection(void *p_client_socket)
{
    int client_socket = *((int *)p_client_socket);
    free(p_client_socket); // frees the pointer so there are no memory leaks

    int joined_room_flag = 0;

    client *current_client;

    char buffer_in[BUFSIZE];

    char input[MAXLINE];

    ssize_t bytesRead;          //number of bytes read
    ssize_t totalBytesRead = 0; // number of total bytes read

    while ((bytesRead = read(client_socket, buffer_in, sizeof(buffer_in) - 1)) > 0)
    {
        buffer_in[bytesRead] = '\0';

        /* Ignore empty buffer */
        if (!strlen(buffer_in))
        {
            continue;
        }

        if (buffer_in[bytesRead - 1] == '\n' || buffer_in[bytesRead - 1] == '\r')
        { // end of the line
            strcat(input, strdup(buffer_in));
            input[totalBytesRead + bytesRead] = '\0';
            totalBytesRead = 0;
            strip_newline(input); // removes the new line character at the end of the input

            if (joined_room_flag)
            {
                if (strlen(input) > 0)
                {
                    char message[MAXLINE];
                    strcat(message, strdup(current_client->name));
                    strcat(message, ": ");
                    strcat(message, strdup(input));
                    strcat(message, "\n");
                    send_message(message, current_client);
                    memset(message, 0, strlen(message));
                }

                memset(input, 0, strlen(input));
                memset(buffer_in, 0, strlen(buffer_in));
            }
            else
            {
                
                if (strlen(input) > 0)
                {
                    // counts total words up to the first 4 words and reads them into an array
                    char *words[3];
                    char *word = strtok(input, " ");
                    int numWords = 0;
                    while (word != NULL && numWords <= 3 && strlen(word) < 20)
                    {
                        words[numWords] = strdup(word);
                        numWords++;
                        word = strtok(NULL, " ");
                    }
                    //
                    if (numWords == 3 && strcmp(words[0], "JOIN") == 0) {
                        // successful join
                        current_client = (client *)malloc(sizeof(client));
                        current_client->socket = client_socket;
                        current_client->room = strdup(words[1]);
                        current_client->name = strdup(words[2]);
                        client_add(current_client);

                        char message[MAXLINE];
                        strcat(message, strdup(current_client->name));
                        strcat(message, " has joined the room\n");
                        send_message(message, current_client);
                        memset(message, 0, strlen(message));

                        joined_room_flag = 1;
                    } else {
                        char message[MAXLINE] = "Invalid command, try 'JOIN {ROOMNAME} {USERNAME}'\n";
                        send(client_socket, message, strlen(message), 0);
                        memset(message, 0, strlen(message));
                    }
                }

            }

            memset(input, 0, strlen(input));
            memset(buffer_in, 0, strlen(buffer_in));
        }
        else if (totalBytesRead + bytesRead > MAXLINE)
        { // case where line exceeds the maximum line value
            totalBytesRead = 0;
            char message[MAXLINE] = "Max line for a message has been exceeded\n";
            send(client_socket, message, strlen(message), 0);

            bzero(&input, sizeof(input));
            bzero(&buffer_in, sizeof(buffer_in));
        }
        else
        {   // still needs to read more from the line
            strcat(input, strdup(buffer_in));
            totalBytesRead += bytesRead;
        }
    }

    if (joined_room_flag) {
        char message[MAXLINE];
        strcat(message, strdup(current_client->name));
        strcat(message, " has left the room\n");
        send_message(message, current_client);
        client_remove(current_client);
        free(current_client);
    }

    pthread_mutex_lock(&clients_mutex);
    num_clients--;
    pthread_mutex_unlock(&clients_mutex);
    
    return NULL;
}

int main(int argc, char *argv[])
{

    int port = 1234;                                   // default port is 1234
    int server_socket, client_socket;                  // sockets for the server and the client
    struct sockaddr_in server_address, client_address; // socket addresses for the server and client
    int addr_size;

    // checks if there was a port given as a command line argument, if so reassigns port
    if (argc == 2)
    {
        port = atoi(argv[1]);
    }
    else if (argc > 2)
    {
        // too many command line arguments, exits the program
        printf("Invalid Command Line Arguments Provided\n");
        return EXIT_FAILURE;
    }

    /* Ignore pipe signals */
    signal(SIGPIPE, SIG_IGN);

    // sets the number of clients to be 0;
    num_clients = 0;

    // creates a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // address for the server socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY is basically address 0.0.0.0 or localhost

    // binds the socket to the specified IP address and port
    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    // listens to the socket with a given backlog of MAX_CLIENTS
    listen(server_socket, MAX_CLIENTS);

    // print statement
    printf("Server Running on port %d...\n", port);

    while (1)
    {
        if (num_clients < MAX_CLIENTS) {
            addr_size = sizeof(server_address);
            client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&addr_size);
            printf("Accepted new client with socket id %d\n", client_socket);
            
            // client connected successfully
            if (client_socket != -1) {
                pthread_mutex_lock(&clients_mutex);
                num_clients++;
                pthread_mutex_unlock(&clients_mutex);
                printf("%d/%d clients have connected to the server\n", num_clients, MAX_CLIENTS);

                // creates a new thread
                pthread_t thread;
                int *pclient = malloc(sizeof(int));
                *pclient = client_socket;
                pthread_create(&thread, NULL, handle_connection, pclient);
            }
        }   

        // reduce overall CPU usage
        sleep(1);

        // int client_socket = accept(server_socket, NULL, NULL);
        // send(client_socket, server_message, sizeof(server_message), 0);
    }

    // closes the server socket
    close(server_socket);
    free_clients();

    return EXIT_SUCCESS;
}

void client_add(client *c)
{
    pthread_mutex_lock(&clients_mutex);
    printf("Adding Client %s\n", c->name);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (!clients[i])
        {
            clients[i] = c;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void list_clients()
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            printf("Room: %s, Client: %s\n", clients[i]->room, clients[i]->name);
            printf("Client: %s, Room: %s\n", clients[i]->name, clients[i]->room);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void client_remove(client *c)
{
    pthread_mutex_lock(&clients_mutex);
    c=NULL;
    pthread_mutex_unlock(&clients_mutex);
}

// Frees all clients in case loop is terminated so there are no memory leaks
void free_clients()
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            free(clients[i]);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients but the sender (cli) */
void send_message(char message[MAXLINE], client *cli)
{
    printf("%s: \n%s", cli->room, message);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i])
        {
            if (strcmp(clients[i]->room, cli->room) == 0 && strcmp(clients[i]->name, cli->name) != 0)
            {
                send(clients[i]->socket, message, strlen(message), 0);
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}
