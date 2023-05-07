#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "../../common/include/utils.h"
#include "../../common/include/request.h"

int executor(const char *command, file_desc fifo, file_desc client_fifo, char *client_fifo_name) {
    char **args = calloc(256, sizeof(char *));
    parse_command(command, args);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();
    if (pid == 0) {
        int ret_val = execvp(args[0], args); // only returns if there is an error (-1)
        if (ret_val == -1) {
            perror("Error executing command");
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    } else if (pid < 0) {
        free(args[0]);
        free(args);
        perror("Fork failed");
        return -1;
    }

    char buffer[32];
    get_timestamp(buffer, sizeof(buffer));
    int ret_val = send_request_and_wait_notification(REQUEST_EXECUTE_START, pid, args[0], buffer, 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error with request and notification exchange\n");

    snprintf(buffer, sizeof(buffer), "Running PID %d\n", pid);
    int written_bytes = write(STDOUT_FILENO, buffer, strlen(buffer));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    int status;
    waitpid(pid, &status, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);

    int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    THROW_ERROR_IF_FAILED_WITH_RETURN(exit_status == -1, "Error executing program\n");

    int elapsed_time = round((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0);
    snprintf(buffer, sizeof(buffer), "Ended in %d ms\n", elapsed_time);
    written_bytes = write(STDOUT_FILENO, buffer, strlen(buffer));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    get_timestamp(buffer, sizeof(buffer));
    ret_val = send_request_and_wait_notification(REQUEST_EXECUTE_END, pid, args[0], buffer, elapsed_time, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error with request and notification exchange\n");

    free(args[0]);
    free(args);

    return exit_status;
}