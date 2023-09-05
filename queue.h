#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct Node {
    int *client_socket;
    struct Node *next;
} Node;

void enqueue(int *client_socket);
int *dequeue();

#endif
