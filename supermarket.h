#ifndef OS2_AS2_SUPERMARKET_H
#define OS2_AS2_SUPERMARKET_H

void supermarket_init(void);
void supermarket_destroy(void);
void *customer_thread(void *arg);
void *cashier_thread(void *arg);
void *terminal_thread(void *arg);

#endif //OS2_AS2_SUPERMARKET_H
