#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include "../include/utils.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

void parse_command(const char *command, char **args) {
    char *cmd_copy = NULL, *token = NULL, *saveptr = NULL;
    int argc = 0;

    cmd_copy = strdup(command);
    THROW_ERROR_IF_FAILED_VOID(cmd_copy == NULL, "Error allocating memory\n");

    while ((token = strsep(&cmd_copy, " ")) != NULL) {
        if (*token == '\0')
            continue; // Skip empty tokens
        args[argc++] = token;
    }
}

void timespec_to_timestamp(struct timespec ts, char *buffer, int size) {
    struct tm *tm = localtime(&ts.tv_sec);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm);
}

int str_to_int(const char *str) {
    char *endptr;
    errno = 0;

    long long_var = strtol(str, &endptr, 10);
    //   out of range   , extra junk at end,  no conversion at all
    if (errno == ERANGE || *endptr != '\0' || str == endptr) {
        perror("Something went wrong converting string to integer!\n");
        exit(EXIT_FAILURE);
    }

    // Needed when `int` and `long` have different ranges
    #if LONG_MIN < INT_MIN || LONG_MAX > INT_MAX
    if (long_var < INT_MIN || long_var > INT_MAX) {
        errno = ERANGE;
        perror("String value is out of range for type integer\n");
        exit(EXIT_FAILURE);
    }
    #endif

    return (int)long_var;
}