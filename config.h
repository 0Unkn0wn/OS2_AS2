#ifndef OS2_AS2_CONFIG_H
#define OS2_AS2_CONFIG_H

// Program constants
#define MAX_CUSTOMERS 10

#define NUM_CARTS 20
#define NUM_SCANNERS 10
#define NUM_CASHIERS 3
#define NUM_TERMINALS 2

// Timing definitions in seconds

#define ARRIVAL_TIME_MIN 2
#define ARRIVAL_TIME_MAX 4

#define SHOPPING_TIME_MIN 1
#define SHOPPING_TIME_MAX 10

#define CASHIER_TIME_MIN 3
#define CASHIER_TIME_MAX 10

#define TERMINAL_TIME 1

#define CART_CLEARING_TIME_MIN 1
#define CART_CLEARING_TIME_MAX 2

#define TERMINAL_CHECKING_TIME_MIN 5
#define TERMINAL_CHECKING_TIME_MAX 10

// catch early errors
#if NUM_CASHIERS < 1
#error "NUM_CASHIERS must be at least 1"
#endif

#if NUM_TERMINALS < 1
#error "NUM_TERMINALS must be at least 1"
#endif

#if NUM_CARTS < 0
#error "NUM_CARTS cannot be negative"
#endif

#if NUM_SCANNERS < 0
#error "NUM_SCANNERS cannot be negative"
#endif

#if MAX_CUSTOMERS < 1
#error "MAX_CUSTOMERS must be at least 1"
#endif

#endif //OS2_AS2_CONFIG_H
