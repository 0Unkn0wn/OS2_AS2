#include "helper_functions.h"

#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sleep_seconds(const double seconds) {
    usleep((useconds_t)(seconds * 1e6));
}

int random_between(int min, int max) {
    return rand() % (max - min + 1) + min;
}

//https://stackoverflow.com/a/23587285 for the print alternative
//not needed anymore because the other threads do not print anything
void safe_print(sem_t *mutex, const char *format, ...) {
    va_list args;
    va_start(args, format);

    sem_wait(mutex);
    vprintf(format, args);
    sem_post(mutex);

    va_end(args);
}