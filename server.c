#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

struct Client {
    int socket;
    char username[50];
};

struct PrivateChatRoom {
    struct Client *participants[2];
};

struct Client clients[MAX_CLIENTS];
struct PrivateChatRoom privateChatRooms[MAX_CLIENTS / 2];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *client_socket) {
    struct Client *cl = (struct Client *)client_socket;
    int sock = cl->socket;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';

        if (strcmp(buffer, "/exit") == 0) {
            printf("Client %s disconnected.\n", cl->username);
            close(sock);
            pthread_exit(NULL);
        } else if (buffer[0] == '/') {
            // Check if it's a private message
            char recipient[50];
            char message[BUFFER_SIZE];
            if (sscanf(buffer, "/private %s %[^\n]", recipient, message) == 2) {
                int recipient_socket = -1;

                // Find the recipient's socket based on their username
                pthread_mutex_lock(&lock);
                for (int k = 0; k < MAX_CLIENTS; k++) {
                    if (clients[k].socket != 0 && strcmp(clients[k].username, recipient) == 0) {
                        recipient_socket = clients[k].socket;
                        break;
                    }
                }
                pthread_mutex_unlock(&lock);

                // If recipient_socket is not -1, it means the recipient was found
                if (recipient_socket != -1 && recipient_socket != sock) {
                    // Send the private message to both participants
                    send(sock, message, strlen(message), 0);
                    send(recipient_socket, message, strlen(message), 0);
                }
            }
        } else {
            // Broadcast the message to all clients, excluding the sender
            pthread_mutex_lock(&lock);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket != 0 && clients[i].socket != sock) {
                    send(clients[i].socket, buffer, strlen(buffer), 0);
                }
            }
            pthread_mutex_unlock(&lock);
        }
    }

    if (read_size == 0) {
        printf("Client %s disconnected.\n", cl->username);
        close(sock);
        pthread_exit(NULL);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket, c;
    struct sockaddr_in server, client;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        strcpy(clients[i].username, "");
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Could not create socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(server_socket, 3);
    printf("Waiting for incoming connections...\n");
    c = sizeof(struct sockaddr_in);

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&c))) {
        printf("Connection accepted.\n");

        pthread_t client_thread;
        struct Client *new_client = NULL;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                new_client = &clients[i];
                break;
            }
        }

        if (new_client == NULL) {
            printf("Maximum number of clients reached. Connection rejected.\n");
            close(client_socket);
        } else {
            new_client->socket = client_socket;

            // Receive the client's username
            if (recv(client_socket, new_client->username, sizeof(new_client->username), 0) <= 0) {
                perror("Username receive failed");
                return 1;
            }

            // Create a new thread to handle the client
            if (pthread_create(&client_thread, NULL, handle_client, (void *)new_client) < 0) {
                perror("Could not create thread");
                return 1;
            }
        }
    }

    if (client_socket < 0) 
    {
        perror("Accept failed");
        return 1;
    }

    return 0;
}
