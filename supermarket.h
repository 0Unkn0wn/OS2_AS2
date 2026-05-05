#ifndef OS2_AS2_SUPERMARKET_H
#define OS2_AS2_SUPERMARKET_H

#include "customer.h"

void supermarket_init(void);
void supermarket_close(void);
void supermarket_destroy(void);

void supermarket_register_customer(Customer *customer, int index);

void *customer_thread(void *arg);
void *cashier_thread(void *arg);
void *terminal_thread(void *arg);
void *display_thread(void *arg);

#endif //OS2_AS2_SUPERMARKET_H
