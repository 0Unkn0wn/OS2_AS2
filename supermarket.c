#include "supermarket.h"
#include "config.h"
#include "queue.h"
#include "helper_functions.h"

#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

static sem_t carts;
static sem_t scanners;

static Queue cashier_queues[NUM_CASHIERS];
static sem_t cashier_queue_mutex[NUM_CASHIERS];
static sem_t cashier_customer_available[NUM_CASHIERS];

static Queue terminal_queue;
static sem_t terminal_queue_mutex;
static sem_t terminal_customer_available;

static sem_t print_mutex;
static sem_t customers_mutex;

static Customer *customers[MAX_CUSTOMERS];
static Customer *cashier_serving[NUM_CASHIERS];
static Customer *terminal_serving[NUM_TERMINALS];

static bool supermarket_running;
static time_t simulation_started_at;

static void join_terminal_queue(Customer *customer);
static void join_shortest_cashier_queue(Customer *customer);
static int find_shortest_cashier_queue(void);
static void set_customer_state(Customer *customer, CustomerState new_state);
static int elapsed_seconds(void);
static const char *state_to_string(CustomerState state);
static char path_char(const Customer *customer);
static void print_display(void);
static void print_customer_line(const Customer *customer, int now);
static void print_queue_customers(const Queue *queue);
static void print_dots(int count);

void supermarket_init(void) {
    supermarket_running = true;
    simulation_started_at = time(NULL);

    sem_init(&carts, 0, NUM_CARTS);
    sem_init(&scanners, 0, NUM_SCANNERS);

    sem_init(&terminal_queue_mutex, 0, 1);
    sem_init(&terminal_customer_available, 0, 0);

    sem_init(&print_mutex, 0, 1);
    sem_init(&customers_mutex, 0, 1);

    queue_init(&terminal_queue);

    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        customers[i] = NULL;
    }

    for (int i = 0; i < NUM_CASHIERS; i++) {
        queue_init(&cashier_queues[i]);
        sem_init(&cashier_queue_mutex[i], 0, 1);
        sem_init(&cashier_customer_available[i], 0, 0);
        cashier_serving[i] = NULL;
    }

    for (int i = 0; i < NUM_TERMINALS; i++) {
        terminal_serving[i] = NULL;
    }
}

void supermarket_close(void) {
    supermarket_running = false;

    for (int i = 0; i < NUM_CASHIERS; i++) {
        sem_post(&cashier_customer_available[i]);
    }

    for (int i = 0; i < NUM_TERMINALS; i++) {
        sem_post(&terminal_customer_available);
    }
}

void supermarket_destroy(void) {
    sem_destroy(&carts);
    sem_destroy(&scanners);

    sem_destroy(&terminal_queue_mutex);
    sem_destroy(&terminal_customer_available);

    sem_destroy(&print_mutex);
    sem_destroy(&customers_mutex);

    for (int i = 0; i < NUM_CASHIERS; i++) {
        sem_destroy(&cashier_queue_mutex[i]);
        sem_destroy(&cashier_customer_available[i]);
    }
}

void supermarket_register_customer(Customer *customer, int index) {
    if (index < 0 || index >= MAX_CUSTOMERS) {
        return;
    }

    sem_wait(&customers_mutex);
    customers[index] = customer;
    sem_post(&customers_mutex);
}

void *cashier_thread(void *arg) {
    int cashier_id = *(int *) arg;

    while (supermarket_running) {
        sem_wait(&cashier_customer_available[cashier_id]);

        if (!supermarket_running) {
            break;
        }

        sem_wait(&cashier_queue_mutex[cashier_id]);
        Customer *customer = queue_pop(&cashier_queues[cashier_id]);
        sem_post(&cashier_queue_mutex[cashier_id]);

        if (customer == NULL) {
            continue;
        }

        sem_wait(&customers_mutex);
        cashier_serving[cashier_id] = customer;
        sem_post(&customers_mutex);

        set_customer_state(customer, STATE_AT_CASHIER);
        sleep_seconds(random_between(CASHIER_TIME_MIN, CASHIER_TIME_MAX));

        sem_wait(&customers_mutex);
        cashier_serving[cashier_id] = NULL;
        sem_post(&customers_mutex);

        sem_post(&customer->done);
    }

    return NULL;
}

void *terminal_thread(void *arg) {
    int terminal_id = *(int *) arg;

    while (supermarket_running) {
        sem_wait(&terminal_customer_available);

        if (!supermarket_running) {
            break;
        }

        sem_wait(&terminal_queue_mutex);
        Customer *customer = queue_pop(&terminal_queue);
        sem_post(&terminal_queue_mutex);

        if (customer == NULL) {
            continue;
        }

        sem_wait(&customers_mutex);
        terminal_serving[terminal_id] = customer;
        sem_post(&customers_mutex);

        set_customer_state(customer, STATE_AT_TERMINAL);
        sleep_seconds(TERMINAL_TIME);

        sem_wait(&customers_mutex);
        terminal_serving[terminal_id] = NULL;
        sem_post(&customers_mutex);

        sem_post(&scanners);
        sem_post(&customer->done);
    }

    return NULL;
}

void *customer_thread(void *arg) {
    Customer *customer = arg;

    if (sem_trywait(&carts) != 0) {
        set_customer_state(customer, STATE_LEFT_NO_CART);
        return NULL;
    }

    customer->has_scanner = false;

    if (random_between(0, 1) == 1) {
        if (sem_trywait(&scanners) == 0) {
            customer->has_scanner = true;
        }
    }

    set_customer_state(customer, STATE_SHOPPING);
    sleep_seconds(random_between(SHOPPING_TIME_MIN, SHOPPING_TIME_MAX));

    if (customer->has_scanner) {
        join_terminal_queue(customer);
    } else {
        join_shortest_cashier_queue(customer);
    }

    sem_wait(&customer->done);

    set_customer_state(customer, STATE_RETURNING_CART);
    sleep_seconds(random_between(CART_CLEARING_TIME_MIN, CART_CLEARING_TIME_MAX));

    sem_post(&carts);

    set_customer_state(customer, STATE_LEFT);

    return NULL;
}

void *display_thread(void *arg) {
    (void) arg;

    while (supermarket_running) {
        print_display();
        sleep_seconds(1.0);
    }

    print_display();
    return NULL;
}

static void join_terminal_queue(Customer *customer) {
    set_customer_state(customer, STATE_WAITING_TERMINAL);

    sem_wait(&terminal_queue_mutex);
    if (!queue_push(&terminal_queue, customer)) {
        sem_post(&terminal_queue_mutex);
        return;
    }
    sem_post(&terminal_queue_mutex);

    sem_post(&terminal_customer_available);
}

static int get_cashier_load(int cashier_id) {
    sem_wait(&cashier_queue_mutex[cashier_id]);
    int load = queue_count(&cashier_queues[cashier_id]);
    sem_post(&cashier_queue_mutex[cashier_id]);

    sem_wait(&customers_mutex);
    if (cashier_serving[cashier_id] != NULL) {
        load++;
    }
    sem_post(&customers_mutex);

    return load;
}

static int find_shortest_cashier_queue(void) {
    int selected_cashier = 0;
    int min_load = get_cashier_load(0);

    for (int i = 1; i < NUM_CASHIERS; i++) {
        int load = get_cashier_load(i);

        if (load < min_load) {
            min_load = load;
            selected_cashier = i;
        }
    }

    return selected_cashier;
}

static void join_shortest_cashier_queue(Customer *customer) {
    int cashier_id = find_shortest_cashier_queue();

    set_customer_state(customer, STATE_WAITING_CASHIER);

    sem_wait(&cashier_queue_mutex[cashier_id]);
    if (!queue_push(&cashier_queues[cashier_id], customer)) {
        sem_post(&cashier_queue_mutex[cashier_id]);
        return;
    }
    sem_post(&cashier_queue_mutex[cashier_id]);

    sem_post(&cashier_customer_available[cashier_id]);
}

static void set_customer_state(Customer *customer, CustomerState new_state) {
    sem_wait(&customers_mutex);

    int now = elapsed_seconds();
    int duration = now - customer->state_started_at;

    switch (customer->state) {
        case STATE_SHOPPING:
            customer->shopping_time += duration;
            break;
        case STATE_WAITING_CASHIER:
        case STATE_WAITING_TERMINAL:
            customer->waiting_time += duration;
            break;
        case STATE_AT_CASHIER:
            customer->cashier_time += duration;
            break;
        case STATE_AT_TERMINAL:
            customer->terminal_time += duration;
            break;
        case STATE_RETURNING_CART:
            customer->returning_time += duration;
            break;
        default:
            break;
    }

    customer->state = new_state;
    customer->state_started_at = now;

    if (new_state == STATE_LEFT || new_state == STATE_LEFT_NO_CART) {
        customer->total_time = customer->shopping_time
                               + customer->waiting_time
                               + customer->cashier_time
                               + customer->terminal_time
                               + customer->returning_time;
    }

    sem_post(&customers_mutex);
}

static int elapsed_seconds(void) {
    return (int) (time(NULL) - simulation_started_at);
}

static const char *state_to_string(CustomerState state) {
    switch (state) {
        case STATE_CREATED:
            return "created";
        case STATE_SHOPPING:
            return "shopping";
        case STATE_WAITING_CASHIER:
        case STATE_WAITING_TERMINAL:
            return "waiting";
        case STATE_AT_CASHIER:
            return "cashier";
        case STATE_AT_TERMINAL:
            return "terminal";
        case STATE_RETURNING_CART:
            return "returning";
        case STATE_LEFT:
            return "done";
        case STATE_LEFT_NO_CART:
            return "left";
        default:
            return "unknown";
    }
}

static char path_char(const Customer *customer) {
    if (customer->state == STATE_LEFT_NO_CART) {
        return '-';
    }

    return customer->has_scanner ? 'S' : 'C';
}

static void print_display(void) {
    sem_wait(&print_mutex);

    printf("\033[2J\033[H");
    printf("Supermarket Checkout Simulator\n");
    printf("Time: %ds\n\n", elapsed_seconds());

    Customer *cashier_snapshot[NUM_CASHIERS];
    Customer *terminal_snapshot[NUM_TERMINALS];

    printf("Customers:\n");
    sem_wait(&customers_mutex);
    int now = elapsed_seconds();
    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        if (customers[i] != NULL) {
            print_customer_line(customers[i], now);
        }
    }
    for (int i = 0; i < NUM_CASHIERS; i++) {
        cashier_snapshot[i] = cashier_serving[i];
    }
    for (int i = 0; i < NUM_TERMINALS; i++) {
        terminal_snapshot[i] = terminal_serving[i];
    }
    sem_post(&customers_mutex);

    printf("\nCashiers:\n");
    for (int i = 0; i < NUM_CASHIERS; i++) {
        sem_wait(&cashier_queue_mutex[i]);
        if (cashier_snapshot[i] == NULL) {
            printf("%d: free        | queue:", i);
        } else {
            printf("%d: serving C%02d | queue:", i, cashier_snapshot[i]->id);
        }
        print_queue_customers(&cashier_queues[i]);
        printf("\n");
        sem_post(&cashier_queue_mutex[i]);
    }

    printf("\nTerminals:\n");
    for (int i = 0; i < NUM_TERMINALS; i++) {
        if (terminal_snapshot[i] == NULL) {
            printf("%d: free\n", i);
        } else {
            printf("%d: serving C%02d\n", i, terminal_snapshot[i]->id);
        }
    }
    sem_wait(&terminal_queue_mutex);
    printf("Terminal queue:");
    print_queue_customers(&terminal_queue);
    printf("\n");
    sem_post(&terminal_queue_mutex);

    int carts_available;
    int scanners_available;
    sem_getvalue(&carts, &carts_available);
    sem_getvalue(&scanners, &scanners_available);

    printf("\nResources:\n");
    printf("Carts:    %2d/%d\n", carts_available, NUM_CARTS);
    printf("Scanners: %2d/%d\n", scanners_available, NUM_SCANNERS);
    fflush(stdout);

    sem_post(&print_mutex);
}

static void print_customer_line(const Customer *customer, int now) {
    const char *state = state_to_string(customer->state);
    char path = path_char(customer);

    if (customer->state == STATE_LEFT_NO_CART) {
        printf("C%02d [%c] | %-12s no cart\n", customer->id, path, state);
        return;
    }

    if (customer->state == STATE_LEFT) {
        printf("C%02d [%c] | %-12s total:%3ds (shopping:%ds, waiting:%ds, cashier:%ds, terminal:%ds, returning:%ds)\n",
               customer->id,
               path,
               state,
               customer->total_time,
               customer->shopping_time,
               customer->waiting_time,
               customer->cashier_time,
               customer->terminal_time,
               customer->returning_time);
        return;
    }

    int seconds_in_state = now - customer->state_started_at;
    printf("C%02d [%c] | %-12s ", customer->id, path, state);
    print_dots(seconds_in_state);
    printf(" %ds\n", seconds_in_state);
}

static void print_queue_customers(const Queue *queue) {
    Node *current = queue->front;

    while (current != NULL) {
        printf(" C%02d", current->customer->id);
        current = current->next;
    }
}

static void print_dots(int count) {
    int dots = count;

    if (dots > 30) {
        dots = 30;
    }

    for (int i = 0; i < dots; i++) {
        putchar('.');
    }
}
