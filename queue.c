#include "queue.h"
#include <stdlib.h>
#include <stdbool.h>

void queue_init(Queue *queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->count = 0;
}

int queue_push(Queue *queue, Customer *customer) {
    Node *node = malloc(sizeof(Node));

    if (node == NULL) {
        return 0;
    }

    node->customer = customer;
    node->next = NULL;

    if (queue->rear == NULL) {
        queue->front = node;
        queue->rear = node;
    } else {
        queue->rear->next = node;
        queue->rear = node;
    }

    queue->count++;

    return 1;
}

Customer *queue_pop(Queue *queue) {
    if (queue->front == NULL) {
        return NULL;
    }

    Node *old_front = queue->front;
    Customer *customer = old_front->customer;

    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(old_front);
    queue->count--;

    return customer;
}

int queue_count(const Queue *queue) {
    return queue->count;
}

bool queue_is_empty(const Queue *queue) {
    return queue->count == 0;
}