#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "../include/array.h"

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

int is_in_array(char **array, char *element, int size);

void get_timestamp(char *buffer, int size);

int get_diff_milliseconds(char *earlier_ts, char *later_ts);

char *strnconcat(char *dest, char *src, int n);

int str_to_int(const char *str);

int readln(file_desc fd, char *line, int size);

int compare_ints(const void *a, const void *b);

Array *get_file_pids(char *dir_path);

void convert_string_array_to_int_array(char **string_array, int *int_array, int size);

#endif
