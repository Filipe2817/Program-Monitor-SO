#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
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

void get_timestamp(char *buffer, int size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t current_time = tv.tv_sec;
    struct tm *local_time = localtime(&current_time);
    snprintf(buffer, size, "%02d-%02d-%04d %02d:%02d:%02d.%03ld",
             local_time->tm_mday, local_time->tm_mon + 1, local_time->tm_year + 1900, 
             local_time->tm_hour, local_time->tm_min, local_time->tm_sec, tv.tv_usec / 1000);
}
