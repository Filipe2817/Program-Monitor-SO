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

pid_t execute_pipeline(char *command, int index, int **pipe_fd, int n_commands) {
    char **args = malloc(32 * sizeof(char *));
    int n_args = parse_command(command, args, " ");
    args[n_args] = NULL;

    pid_t pid = fork();

    if (pid == 0) {
        if (n_commands > 1 && index > 0) {
            dup2(pipe_fd[index - 1][0], STDIN_FILENO);
            close(pipe_fd[index - 1][0]);
        }

        if (index < n_commands - 1) {
            close(pipe_fd[index][0]);
            dup2(pipe_fd[index][1], STDOUT_FILENO);
            close(pipe_fd[index][1]);
        }

        if (execvp(args[0], args) == -1) {
            perror("Exec\n");
        }

        _exit(1);
    } else if (pid < 0) {
        free(args[0]);
        free(args);
        perror("Fork\n");
        return -1;
    }

    if (n_commands > 1 && index > 0) {
        close(pipe_fd[index - 1][0]);
    }

    if (index < n_commands - 1) {
        close(pipe_fd[index][1]);
    }

    free(args[0]);
    free(args);
    return pid;
}

int execute(const char *line, file_desc fifo, file_desc client_fifo, char *client_fifo_name) {
    char **commands = malloc(32 * sizeof(char *));
    int n_commands = parse_command(line, commands, "|");
    int **pipe_fd = malloc((n_commands - 1) * sizeof(int *));

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < n_commands; i++) {
        if (i < n_commands - 1) {
            pipe_fd[i] = malloc(2 * sizeof(int));
            if (pipe(pipe_fd[i]) == -1) {
                perror("Pipe\n");
                exit(EXIT_FAILURE);
            }
        }
        
        pid_t pid = execute_pipeline(commands[i], i, pipe_fd, n_commands);
        
        if (pid == -1) {
            free(commands[0]);
            free(commands);
            perror("Execute failed");
            return -1;
        }
    }

    for (int i = 0; i < n_commands - 1; i++) {
        free(pipe_fd[i]);
    }
    free(pipe_fd);

    char buffer[32];
    get_timestamp(buffer, sizeof(buffer));
    int ret_val = send_request_and_wait_notification(REQUEST_EXECUTE_START, getpid(), commands[0], buffer, 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error with request and notification exchange\n");

    snprintf(buffer, sizeof(buffer), "Running PID %d\n", getpid());
    int written_bytes = write(STDOUT_FILENO, buffer, strlen(buffer));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    for (int i = 0; i < n_commands; i++) {
        int status;
        pid_t terminated_pid = wait(&status);
        int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        THROW_ERROR_IF_FAILED_WITH_RETURN(exit_status == -1, "Error executing program\n");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    int elapsed_time = round((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0);
    snprintf(buffer, sizeof(buffer), "Ended in %d ms\n", elapsed_time);
    written_bytes = write(STDOUT_FILENO, buffer, strlen(buffer));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    get_timestamp(buffer, sizeof(buffer));
    ret_val = send_request_and_wait_notification(REQUEST_EXECUTE_END, getpid(), commands[0], buffer, elapsed_time, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error with request and notification exchange\n");

    free(commands[0]);
    free(commands);

    return exit_status;
}

int execute_status(file_desc fifo, file_desc client_fifo, char *client_fifo_name) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATUS, getpid(), "", "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    int read_bytes_status, read_bytes_size, written_bytes, buffer_size;

    read_bytes_size = read(client_fifo, &buffer_size, sizeof(int));
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_size == -1, "Error reading from fifo.")

    char *status = calloc(buffer_size, sizeof(char));

    read_bytes_status = read(client_fifo, status, buffer_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo.")

    written_bytes = write(STDOUT_FILENO, status, buffer_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    free(status);

    return 0;
}

int execute_stats_time(file_desc fifo, file_desc client_fifo, char *client_fifo_name, char *pids) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATS_TIME, getpid(), pids, "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    int read_bytes_status, written_bytes;

    // read_bytes_size = read(client_fifo, &buffer_size, sizeof(int));
    // THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_size == -1, "Error reading from fifo.")

    char *stats_time = calloc(100, sizeof(char));

    read_bytes_status = read(client_fifo, stats_time, 100);
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo.")

    written_bytes = write(STDOUT_FILENO, stats_time, 100);
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    free(stats_time);

    return 0;
}

int execute_stats_command(file_desc fifo, file_desc client_fifo, char *client_fifo_name, char *command) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATS_COMMAND, getpid(), command, "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    int read_bytes_status, written_bytes;

    // read_bytes_size = read(client_fifo, &buffer_size, sizeof(int));
    // THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_size == -1, "Error reading from fifo.")

    char *stats_com = calloc(100, sizeof(char));

    read_bytes_status = read(client_fifo, stats_com, 100);
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo.")

    written_bytes = write(STDOUT_FILENO, stats_com, 100);
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    free(stats_com);

    return 0;
}

int execute_stats_uniq(file_desc fifo, file_desc client_fifo, char *client_fifo_name) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATS_UNIQ, getpid(), "", "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    int read_bytes_status, written_bytes;

    // read_bytes_size = read(client_fifo, &buffer_size, sizeof(int));
    // THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_size == -1, "Error reading from fifo.")

    char *stats_uniq = calloc(250, sizeof(char));

    read_bytes_status = read(client_fifo, stats_uniq, 250);
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo.")

    written_bytes = write(STDOUT_FILENO, stats_uniq, 250);
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout\n");

    free(stats_uniq);

    return 0;
}
