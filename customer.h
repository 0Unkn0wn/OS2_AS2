#ifndef OS2_AS2_CUSTOMER_H
#define OS2_AS2_CUSTOMER_H

#include <semaphore.h>
#include <stdbool.h>

typedef enum {
    STATE_CREATED,
    STATE_SHOPPING,
    STATE_WAITING_CASHIER,
    STATE_AT_CASHIER,
    STATE_WAITING_TERMINAL,
    STATE_AT_TERMINAL,
    STATE_CHECKING,
    STATE_RETURNING_CART,
    STATE_LEFT,
    STATE_LEFT_NO_CART
} CustomerState;

typedef struct Customer {
    int id;
    bool has_scanner;
    sem_t done;

    CustomerState state;
    int state_started_at;

    int shopping_time;
    int waiting_time;
    int cashier_time;
    int terminal_time;
    int checking_time;
    int returning_time;
    int total_time;
} Customer;

#endif //OS2_AS2_CUSTOMER_H
