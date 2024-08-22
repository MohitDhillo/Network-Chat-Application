#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CACHE_SIZE 5

struct WebPage
{
    char url[1024];
    char content[65536];
    struct WebPage *next;
};

struct Cache
{
    struct WebPage *head;
    int size;
};

void insert_at_front(struct Cache *cache, const char *url, const char *content)
{
    struct WebPage *new_page = (struct WebPage *)malloc(sizeof(struct WebPage));
    strncpy(new_page->url, url, sizeof(new_page->url) - 1);
    strncpy(new_page->content, content, sizeof(new_page->content) - 1);
    new_page->next = cache->head;
    cache->head = new_page;
    cache->size++;

    // Evict the least recently used page if the cache is full
    if (cache->size > CACHE_SIZE)
    {
        struct WebPage *prev = NULL;
        struct WebPage *current = cache->head;
        while (current->next != NULL)
        {
            prev = current;
            current = current->next;
        }
        prev->next = NULL;
        free(current);
        cache->size--;
    }
}

struct WebPage *find_in_cache(struct Cache *cache, const char *url)
{
    struct WebPage *current = cache->head;
    struct WebPage *prev = NULL;

    while (current != NULL)
    {
        if (strcmp(current->url, url) == 0)
        {
            // Move the accessed page to the front (most recently used)
            if (prev != NULL)
            {
                prev->next = current->next;
                current->next = cache->head;
                cache->head = current;
            }
            return current;
        }
        prev = current;
        current = current->next;
    }

    return NULL;
}

void display_cache(struct Cache *cache)
{
    struct WebPage *current = cache->head;
    int c = 1;
    printf("Cache Contents (Most Recently Used to Least Recently Used):\n");
    while (current != NULL)
    {
        printf("%d. %s\n", c++, current->url);
        current = current->next;
    }
}

int main()
{
    struct Cache cache = {NULL, 0};
    int client_socket;
    struct sockaddr_in server_address;
    struct addrinfo hints, *server;
    char input_url[1024];

    while (1)
    {
        printf("Enter the URL of the webpage: ");
        fgets(input_url, sizeof(input_url), stdin);
        input_url[strcspn(input_url, "\n")] = '\0'; // Remove newline

        if (strcmp(input_url, "exit") == 0)
        {
            printf("\nProgram terminated.\n");
            break;
        }

        // Initialize the socket
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Extract the hostname from the URL
        char hostname[1024];
        if (sscanf(input_url, "http://%1023[^/]/", hostname) != 1)
        {
            perror("Invalid URL format");
            exit(EXIT_FAILURE);
        }

        // Set up server_address using getaddrinfo
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(hostname, "80", &hints, &server) != 0)
        {
            perror("Error in resolving hostname");
            exit(EXIT_FAILURE);
        }

        memcpy(&server_address, server->ai_addr, sizeof(struct sockaddr_in));

        freeaddrinfo(server); // Free the server info structure

        // HTTP GET request
        struct WebPage *cached_page = find_in_cache(&cache, input_url);
        if (cached_page != NULL)
        {
            printf("\nALREADY PRESENT in the cache.\n\n%s\n", cached_page->content);
        }
        else
        {
            // Create an HTTP GET request
            printf("\nNOT PRESENT in the cache. GET request needed.\n");
            char request[2048];
            snprintf(request, sizeof(request), "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", hostname);

            // Connect to the server
            if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
            {
                perror("Connection to the server failed");
                exit(EXIT_FAILURE);
            }

            // Send the HTTP request
            send(client_socket, request, strlen(request), 0);

            // Receive and store the response
            char response[65536];
            int bytes_received = 0;
            while (1)
            {
                int bytes = recv(client_socket, response + bytes_received, sizeof(response) - bytes_received, 0);
                if (bytes <= 0)
                {
                    break;
                }
                bytes_received += bytes;
            }

            response[bytes_received] = '\0';

            // Insert the retrieved page into the cache
            insert_at_front(&cache, input_url, response);

            // Display the retrieved content
            printf("\nPage content:\n\n%s\n\n", response);
        }

        // Display the cache contents
        display_cache(&cache);
        printf("\n\n");

        // Clean up and close the socket
        close(client_socket);
    }

    return 0;
}