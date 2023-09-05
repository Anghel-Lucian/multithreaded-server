#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "queue.h"

#define PORT 8080
#define SERVER_CONNECTIONS_BACKLOG 100
#define BUFFER_SIZE 1000000
#define THREAD_POOL_SIZE 20

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t pthread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

// TODO: what if a client sends a connection that is extremely slow? 20 of those and your server will be blocked.
// Find a way to decrease this risk, with Asynchronous I/O or Event-driven programming or otherwise

// utils
bool starts_with(char *prefix, char *string) {
    return strncmp(prefix, string, strlen(prefix)) == 0;
}

char *cpy_request_path(char *src, char *dest, int src_size) {
    int dest_index  = 0;
    bool first_space_encountered = false;

    for (int i = 0; i < src_size; i++) {
        if (src[i - 1] == ' ') {
            if (first_space_encountered) {
                break;
            } else {
                first_space_encountered = true;
            }
        }

        if (first_space_encountered && src[i] != ' ' && src[i] != '/') {
            dest[dest_index] = src[i];
            dest_index++;
        }
    }
}

// Function to handle an individual connection
void *handle_connection(void *arg) {
    int client_file_descriptor = *((int *)arg);
    char *buffer = malloc(BUFFER_SIZE);

    ssize_t bytes_count = recv(client_file_descriptor, buffer, BUFFER_SIZE, 0); 

    printf("Request in thread\n");
    printf("Bytes received: %ld\n", bytes_count);
    printf("Buffer: %s\n", buffer);

    //sleep(1); // TODO: remove, keep to test threads

    if (bytes_count > 0) {
        char resource[100];
        cpy_request_path(buffer, resource, BUFFER_SIZE);
        printf("%s\n", resource);
    }

    close(client_file_descriptor);
    free(arg);
    free(buffer);

    return NULL;
}

// Function to run on each one of the threads of the thread pool
void *thread_function() {
    while (true) {
        pthread_mutex_lock(&pthread_mutex);

        int *client_file_descriptor;
        
        if ((client_file_descriptor = dequeue()) == NULL) {
            pthread_cond_wait(&condition_var, &pthread_mutex);
            
            client_file_descriptor = dequeue(); 
        }

        pthread_mutex_unlock(&pthread_mutex);

        if (client_file_descriptor != NULL) {
            handle_connection(client_file_descriptor);
        }
    }
}

int main(int argc, char *argv[]) {
    bool multi_threaded = true;
    int port = PORT;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            char *arg = argv[i];

            if (strcmp(arg, "-st") == 0) {
                multi_threaded = false;
            }

            if (starts_with("-p=", arg) || starts_with("-port=", arg)) {
                char *arg_port = arg + 3;
                port = atoi(arg_port); 
            }
        }
    }

    int server_file_descriptor;
    struct sockaddr_in server_addr;

    server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    if (server_file_descriptor < 0) {
        perror("Socket initialization failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_file_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Socket binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_file_descriptor, SERVER_CONNECTIONS_BACKLOG) < 0) {
        perror("Socket listening failed");
        exit(EXIT_FAILURE);
    }

    if (multi_threaded) {
        printf("Starting multi-threaded server\n");
        for (int i = 0; i < THREAD_POOL_SIZE; i++) {
            pthread_create(&thread_pool[i], NULL, thread_function, NULL);
        }
    } else {
        printf("Starting single-threaded server\n");
    }
    printf("Server listening on port %d\n", port);

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_file_descriptor = malloc(sizeof(int));

        *client_file_descriptor = accept(server_file_descriptor, (struct sockaddr *)&client_addr, &client_addr_len);

        if (*client_file_descriptor < 0) {
            perror("Request accept failed");
            continue;
        }

        if (multi_threaded) {
            // TODO: threads take up memory, what if you have 1000000 connections? You might see severe performance issues even if you have more threads
            // creating threads takes up time. Can you think of a solution where threads are already created? -> Thread pool
            //pthread_t thread_id;
            //pthread_create(&thread_id, NULL, handle_connection, (void *)client_file_descriptor);
            //pthread_detach(thread_id);
            pthread_mutex_lock(&pthread_mutex);
            enqueue(client_file_descriptor);
            pthread_cond_signal(&condition_var);
            pthread_mutex_unlock(&pthread_mutex);
        } else {
            handle_connection((void *)client_file_descriptor);
        }
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_detach(thread_pool[i]);
    }

    free(thread_pool);

    close(server_file_descriptor);
    return 0;
}

