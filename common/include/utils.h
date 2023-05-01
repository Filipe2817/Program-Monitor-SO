#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define FP

#ifdef FP
#define FIFO_NAME "/home/fp/fifos/Tracer-Monitor"
#else
#define FIFO_NAME "Tracer-Monitor"
#endif

/*
Este define é para apagar quando for para entregar
Preciso daquele path especifico para usar fifos no meu PC
*/

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
