#include "server.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <sys/select.h>

static int num_clients;
pthread_mutex_t num_clients_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t stop_server = 0;
connection_t connection_infos[MAX_CLIENTS];

void sigint_handler(int signum)
{
    printf("\nShutting down server...\n");
    stop_server = 1;

    // Close all active client connections
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (connection_infos[i].is_connected == 1)
        {
            connection_infos[i].is_connected = 0;
            close(connection_infos[i].socket);
        }
    }
}

void *handle_client(void *arg)
{
    connection_t *conn_info = (connection_t *)arg;

    char message[MAX_MESSAGE_LENGTH];
    snprintf(message, MAX_MESSAGE_LENGTH, "Welcome!\nType 'help' for commands.\n");
    send(conn_info->socket, message, strlen(message), 0);

    char buffer[MAX_MESSAGE_LENGTH + 1];
    while (1)
    {
        ssize_t bytes = 0, total_bytes = 0;
        while (total_bytes < MAX_MESSAGE_LENGTH - 1)
        {
            bytes = recv(conn_info->socket, buffer + total_bytes, 1, 0);
            if (bytes <= 0)
            {
                total_bytes = bytes;
                break;
            }
            if (buffer[total_bytes] == '\n')
            {
                total_bytes++;
                break;
            }
            total_bytes++;
        }
        if (total_bytes <= 0)
        {
            break;
        }
        buffer[total_bytes] = '\0';

        if (conn_info->is_in_room && buffer[0] != '\n')
        {
            // send everything in the buffer to everyone in the room except you
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (connection_infos[i].is_connected && connection_infos[i].is_in_room &&
                    strcmp(connection_infos[i].roomname, conn_info->roomname) == 0 &&
                    connection_infos[i].socket != conn_info->socket)
                {
                    char message_with_username[MAX_MESSAGE_LENGTH + MAX_USERNAME_LENGTH + 3];
                    snprintf(message_with_username, sizeof(message_with_username), "%s: %s", conn_info->username, buffer);
                    send(connection_infos[i].socket, message_with_username, strlen(message_with_username), 0);
                }
            }
            continue;
        }

        char *command = strtok(buffer, " \r\n");

        if (command == NULL)
        {
            continue;
        }

        if (strcasecmp(command, "help") == 0)
        {
            const char help_message[] =
                "Available commands:\n"
                "\thelp - Show this message\n"
                "\tjoin {roomname} {username} - Join a room with a username\n"
                "\tquit - Disconnect from the server\n"
                "\texit - Disconnect from the server\n";
            send(conn_info->socket, help_message, strlen(help_message), 0);
        }
        else if (strcasecmp(command, "exit") == 0 || strcasecmp(command, "quit") == 0)
        {
            const char exit_message[] = "Goodbye!\n";
            send(conn_info->socket, exit_message, strlen(exit_message), 0);
            break;
        }
        else if (strcasecmp(command, "join") == 0)
        {
            char *room_name = NULL;
            char *user_name = NULL;
            if (total_bytes > 0 && total_bytes < MAX_MESSAGE_LENGTH &&
                (room_name = strtok(NULL, " \r\n")) != NULL &&
                (user_name = strtok(NULL, " \r\n")) != NULL)
            {
            }
            else
            {
                send(conn_info->socket, "Usage: join <room_name> <user_name>\n", 37, 0);
                continue;
            }

            int room_name_len = strlen(room_name);
            int user_name_len = strlen(user_name);

            if (room_name_len > MAX_ROOMNAME_LENGTH)
            {
                char error_message[256];
                snprintf(error_message, sizeof(error_message),
                         "Room name '%s' is too long. Max length is: %d.\n",
                         room_name, MAX_ROOMNAME_LENGTH);
                send(conn_info->socket, error_message, strlen(error_message), 0);
                continue;
            }

            if (user_name_len > MAX_USERNAME_LENGTH)
            {
                char error_message[256];
                snprintf(error_message, sizeof(error_message),
                         "Username '%s' is too long. Max length is: %d.\n",
                         user_name, MAX_USERNAME_LENGTH);
                send(conn_info->socket, error_message, strlen(error_message), 0);
                continue;
            }

            char join_message[256];
            snprintf(join_message, sizeof(join_message), "Joining room %s as %s\n", room_name, user_name);
            send(conn_info->socket, join_message, strlen(join_message), 0);

            conn_info->is_in_room = 1;
            strncpy(conn_info->roomname, room_name, MAX_ROOMNAME_LENGTH);
            strncpy(conn_info->username, user_name, MAX_USERNAME_LENGTH);
        }
        else
        {
            const char error_message[] = "Unknown command. Type 'help' for available commands.\n";
            send(conn_info->socket, error_message, strlen(error_message), 0);
        }
    }

    close(conn_info->socket);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(conn_info->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    pthread_mutex_lock(&num_clients_mutex);
    conn_info->is_connected = 0;
    num_clients--;
    printf("Client %s:%d Disconnected\t%d/%d Connections Active\n", client_ip, ntohs(conn_info->address.sin_port), num_clients, MAX_CLIENTS);
    pthread_mutex_unlock(&num_clients_mutex);

    return NULL;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    if (argc > 3)
    {
        printf("Invalid Command Line Arguments Provided\nUse the format './server [hostname] [port]'\n");
        return EXIT_FAILURE;
    }

    if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
        printf("Usage: ./server [hostname] [port]\n");
        printf("Options:\n");
        printf("  -h          Show this help message and exit\n");
        printf("  --version   Show the version information and exit\n");
        return EXIT_SUCCESS;
    }

    if (argc == 2 && strcmp(argv[1], "--version") == 0)
    {
        printf("Simple Chat Server Version: 1.0.0\n");
        return EXIT_SUCCESS;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    int port = DEFAULT_PORT;

    if (argc >= 2)
    {
        if (strcmp(argv[1], "localhost") == 0)
        {
            server_address.sin_addr.s_addr = INADDR_ANY;
        }
        else if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0)
        {
            printf("Invalid host %s\n", argv[1]);
            server_address.sin_addr.s_addr = INADDR_ANY;
        }
    }
    else
    {
        server_address.sin_addr.s_addr = INADDR_ANY;
    }

    if (argc == 3)
    {
        port = atoi(argv[2]);
    }
    server_address.sin_port = htons(port);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_socket);
        return EXIT_FAILURE;
    }
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Error binding socket");
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (listen(server_socket, 10) < 0)
    {
        perror("Error listening on socket");
        close(server_socket);
        return EXIT_FAILURE;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_address.sin_addr, ip_str, INET_ADDRSTRLEN);
    printf("Chat server active on %s:%d\n", ip_str, port);

    fd_set read_fds;
    int max_fd = server_socket;

    while (!stop_server)
    {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (activity < 0)
        {
            if (errno == EINTR)
                break;
            perror("Error with select");
            break;
        }

        if (FD_ISSET(server_socket, &read_fds))
        {
            struct sockaddr_in client_address;
            socklen_t client_address_len = sizeof(client_address);
            int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
            if (client_socket < 0)
            {
                perror("Error accepting connection");
                continue;
            }

            pthread_mutex_lock(&num_clients_mutex);
            if (num_clients >= MAX_CLIENTS)
            {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
                printf("Client %s:%d Rejected   \t%d/%d Connections Active\n", client_ip, ntohs(client_address.sin_port), num_clients, MAX_CLIENTS);
                pthread_mutex_unlock(&num_clients_mutex);
                char msg[] = "Server full. Try again later.\n";
                send(client_socket, msg, strlen(msg), 0);
                close(client_socket);
                continue;
            }
            num_clients++;
            pthread_mutex_unlock(&num_clients_mutex);

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
            printf("Client %s:%d Accepted   \t%d/%d Connections Active\n", client_ip, ntohs(client_address.sin_port), num_clients, MAX_CLIENTS);
            pthread_t thread;

            int connection_index = -1;

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (connection_infos[i].is_connected == 0)
                {
                    connection_index = i;
                    break;
                }
            }

            if (connection_index == -1)
            {
                printf("No available connection slots.\n");
                close(client_socket);
                continue;
            }

            connection_infos[connection_index] = (connection_t){
                .socket = client_socket,
                .address = client_address,
                .is_connected = 1,
                .is_in_room = 0};

            pthread_create(&thread, NULL, handle_client, (void *)&connection_infos[connection_index]);
            pthread_detach(thread);
        }
    }

    close(server_socket);
    printf("Server socket closed. Goodbye!\n");
    return EXIT_SUCCESS;
}
