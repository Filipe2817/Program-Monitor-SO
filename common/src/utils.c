#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "../include/utils.h"

int createNewFifo(const char *fifo_name) {
    struct stat stats;

    if (stat(fifo_name, &stats) < 0) { // Stat failed
        // If (errno == ENOENT), the file or directory does not exist
        // If (errno != ENOENT), an error occurred that is unrelated to the file or directory not existing (Stat failed)
        THROW_ERROR_IF_FAILED_WITH_RETURN(errno != ENOENT, "Stat failed\n");
    } else {
        // The fifo already exists so we need to delete it
        THROW_ERROR_IF_FAILED_WITH_RETURN(unlink(fifo_name) < 0, "Unlink failed\n");
    }

    // Create new fifo
    THROW_ERROR_IF_FAILED_WITH_RETURN(mkfifo(fifo_name, 0666) < 0, "Fifo creation failed\n");

    return 0;
}

void parse_command(const char *command, char **args) {
    char *cmd_copy = NULL, *token = NULL;
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
