#include <stdio.h>
#include <stdarg.h>

#ifndef UTILS_H
#define UTILS_H

/*
Este define é para apagar quando for para entregar
Preciso daquele path especifico para usar fifos no meu PC
*/

typedef int file_desc;

#define THROW_ERROR_IF_FAILED_WITH_RETURN(expression, msg) \
    if (expression)                                        \
    {                                                      \
        perror(msg);                                       \
        return -1;                                         \
    }

#define THROW_ERROR_IF_FAILED_VOID(expression, msg) \
    if (expression)                                 \
    {                                               \
        perror(msg);                                \
        return;                                     \
    }

int createNewFifo(const char *fifo_name);

void parse_command(const char *command, char **args);

void get_timestamp(char *buffer, int size);

#endif
