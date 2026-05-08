#ifndef OS2_AS2_QUEUE_H
#define OS2_AS2_QUEUE_H

#include "customer.h"
#include <stdbool.h>

typedef struct Node {
    Customer *customer;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
    int count;
} Queue;

void queue_init(Queue *queue);

int queue_push(Queue *queue, Customer *customer);

Customer *queue_pop(Queue *queue);

int queue_count(const Queue *queue);

bool queue_is_empty(const Queue *queue);

#endif //OS2_AS2_QUEUE_H
