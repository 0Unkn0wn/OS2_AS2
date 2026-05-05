#include "config.h"
#include "customer.h"
#include "supermarket.h"
#include "helper_functions.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned int) time(NULL));

    supermarket_init();

    pthread_t display;
    pthread_create(&display, NULL, display_thread, NULL);

    pthread_t cashier_threads[NUM_CASHIERS];
    int cashier_ids[NUM_CASHIERS];

    for (int i = 0; i < NUM_CASHIERS; i++) {
        cashier_ids[i] = i;
        pthread_create(&cashier_threads[i], NULL, cashier_thread, &cashier_ids[i]);
    }

    pthread_t terminal_threads[NUM_TERMINALS];
    int terminal_ids[NUM_TERMINALS];

    for (int i = 0; i < NUM_TERMINALS; i++) {
        terminal_ids[i] = i;
        pthread_create(&terminal_threads[i], NULL, terminal_thread, &terminal_ids[i]);
    }

    pthread_t customer_threads[MAX_CUSTOMERS];
    Customer *customers[MAX_CUSTOMERS];

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        Customer *customer = malloc(sizeof(Customer));

        if (customer == NULL) {
            customers[i] = NULL;
            continue;
        }

        customers[i] = customer;

        customer->id = i + 1;
        customer->has_scanner = false;
        customer->state = STATE_CREATED;
        customer->state_started_at = 0;
        customer->shopping_time = 0;
        customer->waiting_time = 0;
        customer->cashier_time = 0;
        customer->terminal_time = 0;
        customer->returning_time = 0;
        customer->total_time = 0;
        sem_init(&customer->done, 0, 0);

        supermarket_register_customer(customer, i);
        pthread_create(&customer_threads[i], NULL, customer_thread, customer);

        if (i < MAX_CUSTOMERS - 1) { // do not wait for another customer after the last one we are closing the store
            sleep_seconds(random_between(ARRIVAL_TIME_MIN, ARRIVAL_TIME_MAX));
        }
    }

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        if (customers[i] != NULL) {
            pthread_join(customer_threads[i], NULL);
        }
    }

    supermarket_close();

    for (int i = 0; i < NUM_CASHIERS; i++) {
        pthread_join(cashier_threads[i], NULL);
    }

    for (int i = 0; i < NUM_TERMINALS; i++) {
        pthread_join(terminal_threads[i], NULL);
    }

    pthread_join(display, NULL);

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        if (customers[i] != NULL) {
            sem_destroy(&customers[i]->done);
            free(customers[i]);
        }
    }

    supermarket_destroy();

    return 0;
}
