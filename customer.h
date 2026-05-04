#ifndef OS2_AS2_CUSTOMER_H
#define OS2_AS2_CUSTOMER_H

#include <semaphore.h>
#include <stdbool.h>

typedef struct Customer {
    int id;
    bool has_scanner;
    sem_t done;
} Customer;

#endif //OS2_AS2_CUSTOMER_H
