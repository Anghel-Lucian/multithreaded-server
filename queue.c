#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

Node *head;

void enqueue(int *client_socket) {
    printf("enqueue\n");
    Node *new_node = malloc(sizeof(Node));

    new_node->client_socket = client_socket;
    new_node->next = NULL;

    if (head == NULL) {
        head = new_node;
    } else {
        Node *ptr = head;

        while (ptr->next != NULL) {
            ptr = ptr->next;
        }

        ptr->next = new_node;
    }
}

int *dequeue() {
    printf("dequeue\n");
    Node *dequeued_node = head;

    if (dequeued_node == NULL) {
        return NULL;
    } 

    head = head->next;

    int *client_socket = dequeued_node->client_socket;

    free(dequeued_node);

    return client_socket;
}

