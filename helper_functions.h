#ifndef OS2_AS2_HELPER_FUNCTIONS_H
#define OS2_AS2_HELPER_FUNCTIONS_H
#include <semaphore.h>

void sleep_seconds(double seconds);

int random_between(int min, int max);

void safe_print(sem_t *mutex, const char *format, ...);

#endif //OS2_AS2_HELPER_FUNCTIONS_H
