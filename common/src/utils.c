#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include "../include/utils.h"

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