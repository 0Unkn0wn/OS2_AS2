#include "config.h"
#include "customer.h"
#include "supermarket.h"
#include "helper_functions.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned int) time(NULL));

    supermarket_init();

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

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        Customer *customer = malloc(sizeof(Customer));

        if (customer == NULL) {
            continue;
        }

        customer->id = i + 1;
        customer->has_scanner = false;
        sem_init(&customer->done, 0, 0);

        pthread_create(&customer_threads[i], NULL, customer_thread, customer);

        sleep_seconds(random_between(ARRIVAL_TIME_MIN, ARRIVAL_TIME_MAX));
    }

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        pthread_join(customer_threads[i], NULL);
    }

    supermarket_destroy();

    return 0;
}