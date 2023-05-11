#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#ifndef UTILS_H
#define UTILS_H

/*
Este define é para apagar quando for para entregar
Preciso daquele path especifico para usar fifos no meu PC
*/

typedef int file_desc;

#define THROW_ERROR_IF_FAILED_WITH_RETURN(expression, msg) \
    if (expression) {                                      \
        perror(msg);                                       \
        return -1;                                         \
    }

#define THROW_ERROR_IF_FAILED_VOID(expression, msg) \
    if (expression) {                               \
        perror(msg);                                \
        return;                                     \
    }

int create_new_fifo(const char *fifo_name);

int parse_command(const char *command, char **args, char *sep);

bool found_in(char **list, char *id);

void get_timestamp(char *buffer, int size);

int get_diff_milliseconds(char *earlier_ts, char *later_ts);

char *str_concat(char *dest, char *src);

#endif
