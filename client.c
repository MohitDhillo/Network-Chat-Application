// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <pthread.h>

// #define BUFFER_SIZE 1024

// void *receive_messages(void *socket) {
//     int sock = *(int *)socket;
//     char buffer[BUFFER_SIZE];
//     int read_size;

//     while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
//         buffer[read_size] = '\0';
//         printf("%s\n", buffer);
//     }

//     return 0;
// }

// int main(int argc, char *argv[]) {
//     int sock;
//     struct sockaddr_in server;
//     char message[BUFFER_SIZE];
//     char username[50];

//     sock = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock == -1) {
//         perror("Could not create socket");
//         return 1;
//     }

//     server.sin_addr.s_addr = inet_addr("127.0.0.1");
//     server.sin_family = AF_INET;
//     server.sin_port = htons(8888);

//     if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
//         perror("Connection failed");
//         return 1;
//     }

//     printf("Connected to server.\n");

//     printf("Enter your username: ");
//     fgets(username, sizeof(username), stdin);
//     username[strcspn(username, "\n")] = '\0'; // Remove newline character

//     // Send the username to the server
//     if (send(sock, username, sizeof(username), 0) <= 0) {
//         perror("Username send failed");
//         return 1;
//     }

//     pthread_t receive_thread;
//     if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&sock) < 0) {
//         perror("Could not create thread");
//         return 1;
//     }

//     while (1) {
//         fgets(message, BUFFER_SIZE, stdin);

//         if (strncmp(message, "/exit", 5) == 0) {
//             send(sock, message, strlen(message), 0);
//             break;
//         } else if (strncmp(message, "/private", 8) == 0) {
//             // Extract the recipient's username and message
//             char recipient[50];
//             int i = 8;
//             while (message[i] == ' ') {
//                 i++;
//             }
//             int j = 0;
//             while (message[i] != ' ' && message[i] != '\n') {
//                 recipient[j++] = message[i++];
//             }
//             recipient[j] = '\0';

//             // Send the private chat request
//             // snprintf(message, sizeof(message), "/private %s %s", recipient, message + i);
//             // Send the private chat request
//             char private_request[BUFFER_SIZE];
//             snprintf(private_request, sizeof(private_request), "/private %s %s", recipient, message + i);
//             send(sock, private_request, strlen(private_request), 0);

//         }

//         // Send message with the user's username
//         char message_with_username[BUFFER_SIZE];
//         snprintf(message_with_username, sizeof(message_with_username), "%s: %s", username, message);
//         send(sock, message_with_username, strlen(message_with_username), 0);
//     }

//     close(sock);
//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *receive_messages(void *socket) 
{
    
    int sock = *(int *)socket;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) 
    {
        buffer[read_size] = '\0';
        printf("%s\n", buffer);
    }
    // ... (existing code)

    return NULL;
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];
    char username[50];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to server.\n");

    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0'; // Remove newline character

    // Send the username to the server
    if (send(sock, username, sizeof(username), 0) <= 0) {
        perror("Username send failed");
        return 1;
    }

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&sock) < 0) {
        perror("Could not create thread");
        return 1;
    }

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);

        if (strncmp(message, "/exit", 5) == 0) {
            send(sock, message, strlen(message), 0);
            break;
        } else if (strncmp(message, "/private", 8) == 0) {
            // Extract the recipient's username and message
            char recipient[50];
            int i = 8;
            while (message[i] == ' ') {
                i++;
            }
            int j = 0;
            while (message[i] != ' ' && message[i] != '\n') {
                recipient[j++] = message[i++];
            }
            recipient[j] = '\0';

            // Check if the message contains both recipient and content
            if (message[i] == ' ') {
                i++;
                // Send the private chat request with the correct format
                char private_request[BUFFER_SIZE];
                snprintf(private_request, sizeof(private_request), "/private %s %s", recipient, message + i);
                send(sock, private_request, strlen(private_request), 0);
            }
        } else {
            // Send message with the user's username
            char message_with_username[BUFFER_SIZE];
            snprintf(message_with_username, sizeof(message_with_username), "%s: %s", username, message);
            send(sock, message_with_username, strlen(message_with_username), 0);
        }
    }

    close(sock);
    return 0;
}
