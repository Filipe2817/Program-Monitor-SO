#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define THROW_ERROR_IF_FAILED_WITH_RETURN(expression, msg) \
    if (expression) {                                      \
        perror(msg);                                       \
        return -1;                                         \
    }

#define THROW_ERROR_IF_FAILED_VOID(expression, msg) \
    if (expression) {                              \
        perror(msg);                               \
        return;                                    \
    }

#endif