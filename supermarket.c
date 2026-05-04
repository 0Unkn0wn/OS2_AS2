#include "supermarket.h"
#include "config.h"
#include "queue.h"

#include <semaphore.h>
#include <stdlib.h>

#include "helper_functions.h"

static sem_t carts;
static sem_t scanners;

static Queue cashier_queues[NUM_CASHIERS];
static sem_t cashier_queue_mutex[NUM_CASHIERS];
static sem_t cashier_customer_available[NUM_CASHIERS];

static Queue terminal_queue;
static sem_t terminal_queue_mutex;
static sem_t terminal_customer_available;

static sem_t print_mutex;


static void join_terminal_queue(Customer *customer);
static void join_shortest_cashier_queue(Customer *customer);


void supermarket_init(void) {
    sem_init(&carts, 0, NUM_CARTS);
    sem_init(&scanners, 0, NUM_SCANNERS);

    sem_init(&terminal_queue_mutex, 0, 1);
    sem_init(&terminal_customer_available, 0, 0);

    sem_init(&print_mutex, 0, 1);

    queue_init(&terminal_queue);

    for (int i = 0; i < NUM_CASHIERS; i++) {
        queue_init(&cashier_queues[i]);
        sem_init(&cashier_queue_mutex[i], 0, 1);
        sem_init(&cashier_customer_available[i], 0, 0);
    }
}

void supermarket_destroy(void) {
    sem_destroy(&carts);
    sem_destroy(&scanners);

    sem_destroy(&terminal_queue_mutex);
    sem_destroy(&terminal_customer_available);

    sem_destroy(&print_mutex);

    for (int i = 0; i < NUM_CASHIERS; i++) {
        sem_destroy(&cashier_queue_mutex[i]);
        sem_destroy(&cashier_customer_available[i]);
    }
}

void *cashier_thread(void *arg) {
    int cashier_id = *(int *)arg;

    while (true) {
        sem_wait(&cashier_customer_available[cashier_id]);

        sem_wait(&cashier_queue_mutex[cashier_id]);
        Customer *customer = queue_pop(&cashier_queues[cashier_id]);
        sem_post(&cashier_queue_mutex[cashier_id]);

        if (customer == NULL) {
            continue;
        }

        safe_print(&print_mutex,
                   "Cashier %d is serving customer %d\n",
                   cashier_id,
                   customer->id);

        sleep_seconds(random_between(CASHIER_TIME_MIN, CASHIER_TIME_MAX));

        safe_print(&print_mutex,
                   "Cashier %d finished customer %d\n",
                   cashier_id,
                   customer->id);

        sem_post(&customer->done);
    }

    return NULL;
}

void *terminal_thread(void *arg) {
    int terminal_id = *(int *)arg;

    while (true) {
        sem_wait(&terminal_customer_available);

        sem_wait(&terminal_queue_mutex);
        Customer *customer = queue_pop(&terminal_queue);
        sem_post(&terminal_queue_mutex);

        if (customer == NULL) {
            continue;
        }

        safe_print(&print_mutex,
                   "Terminal %d is serving customer %d\n",
                   terminal_id,
                   customer->id);

        sleep_seconds(TERMINAL_TIME);

        safe_print(&print_mutex,
                   "Terminal %d finished customer %d\n",
                   terminal_id,
                   customer->id);

        sem_post(&scanners);
        sem_post(&customer->done);
    }

    return NULL;
}

void *customer_thread(void *arg) {
    Customer *customer = arg;

    safe_print(&print_mutex,
               "Customer %d arrived\n",
               customer->id);

    if (sem_trywait(&carts) != 0) {
        safe_print(&print_mutex,
                   "Customer %d left because no cart was available\n",
                   customer->id);

        sem_destroy(&customer->done);
        free(customer);
        return NULL;
    }

    safe_print(&print_mutex,
               "Customer %d took a cart\n",
               customer->id);
    customer->has_scanner = false;

    if (random_between(0, 1) == 1) {
        if (sem_trywait(&scanners) == 0) {
            customer->has_scanner = true;

            safe_print(&print_mutex,
                       "Customer %d took a scanner\n",
                       customer->id);
        } else {
            safe_print(&print_mutex,
                       "Customer %d wanted a scanner but none were available\n",
                       customer->id);
        }
    }
    safe_print(&print_mutex,
           "Customer %d is shopping\n",
           customer->id);

    sleep_seconds(random_between(SHOPPING_TIME_MIN, SHOPPING_TIME_MAX));

    if (customer->has_scanner) {
        join_terminal_queue(customer);
    } else {
        join_shortest_cashier_queue(customer);
    }
    sem_wait(&customer->done);
    safe_print(&print_mutex,
           "Customer %d is returning cart\n",
           customer->id);

    sleep_seconds(random_between(CART_CLEARING_TIME_MIN, CART_CLEARING_TIME_MAX));

    sem_post(&carts);

    safe_print(&print_mutex,
               "Customer %d left supermarket\n",
               customer->id);

    sem_destroy(&customer->done);
    free(customer);

    return NULL;
}

static void join_terminal_queue(Customer *customer) {
    sem_wait(&terminal_queue_mutex);
    queue_push(&terminal_queue, customer);
    int count = queue_count(&terminal_queue);
    sem_post(&terminal_queue_mutex);

    safe_print(&print_mutex,
               "Customer %d joined terminal queue (length %d)\n",
               customer->id, count);

    sem_post(&terminal_customer_available);
}

static int find_shortest_cashier_queue(void) {
    int best_index = 0;
    int best_count;

    sem_wait(&cashier_queue_mutex[0]);
    best_count = queue_count(&cashier_queues[0]);
    sem_post(&cashier_queue_mutex[0]);

    for (int i = 1; i < NUM_CASHIERS; i++) {
        sem_wait(&cashier_queue_mutex[i]);
        int count = queue_count(&cashier_queues[i]);
        sem_post(&cashier_queue_mutex[i]);

        if (count < best_count) {
            best_count = count;
            best_index = i;
        }
    }

    return best_index;
}

static void join_shortest_cashier_queue(Customer *customer) {
    int cashier_id = find_shortest_cashier_queue();

    sem_wait(&cashier_queue_mutex[cashier_id]);
    queue_push(&cashier_queues[cashier_id], customer);
    int count = queue_count(&cashier_queues[cashier_id]);
    sem_post(&cashier_queue_mutex[cashier_id]);

    safe_print(&print_mutex,
               "Customer %d joined cashier %d queue (length %d)\n",
               customer->id, cashier_id, count);

    sem_post(&cashier_customer_available[cashier_id]);
}