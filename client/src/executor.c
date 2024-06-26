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
            perror("Exec");
        }

        _exit(1);
    } else if (pid < 0) {
        free(args[0]);
        free(args);
        perror("Fork");
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
    int **pipe_fd = NULL;
    char *commands_names = malloc(256 * sizeof(char)), *ptr = commands_names;
    commands_names[0] = '\0';
    
    if (n_commands > 1) {
        pipe_fd = malloc((n_commands - 1) * sizeof(int *));
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < n_commands; i++) {
        if (i < n_commands - 1) {
            pipe_fd[i] = malloc(2 * sizeof(int));
            THROW_ERROR_IF_FAILED_WITH_RETURN(pipe(pipe_fd[i]) == -1, "Error creating pipe");
        }
        
        if (i > 0) {
            ptr = strnconcat(ptr, "| ", 2);
        } 
        ptr = strnconcat(ptr, commands[i], strcspn(commands[i], " "));

        pid_t pid = execute_pipeline(commands[i], i, pipe_fd, n_commands);
        
        if (pid == -1) {
            free(commands[0]);
            free(commands);
            perror("Execute failed");
            return -1;
        }
    }
    commands_names[ptr - commands_names] = '\0';

    for (int i = 0; i < n_commands - 1; i++) {
        free(pipe_fd[i]);
    }
    free(pipe_fd);

    char buffer[32];
    get_timestamp(buffer, sizeof(buffer));
    int ret_val = send_request_and_wait_notification(REQUEST_EXECUTE_START, getpid(), commands_names, buffer, 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error with request and notification exchange");

    snprintf(buffer, sizeof(buffer), "Running PID %d\n", getpid());
    int written_bytes = write(STDOUT_FILENO, buffer, strlen(buffer));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout");

    for (int i = 0; i < n_commands; i++) {
        int status;
        (void) wait(&status);
        int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        THROW_ERROR_IF_FAILED_WITH_RETURN(exit_status == -1, "Error executing program");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    int elapsed_time = round((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0);
    snprintf(buffer, sizeof(buffer), "Ended in %d ms\n", elapsed_time);
    written_bytes = write(STDOUT_FILENO, buffer, strlen(buffer));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout");

    get_timestamp(buffer, sizeof(buffer));
    ret_val = send_request_and_wait_notification(REQUEST_EXECUTE_END, getpid(), commands_names, buffer, elapsed_time, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error with request and notification exchange");

    free(commands[0]);
    free(commands);
    free(commands_names);
    return 0;
}

int execute_status(file_desc fifo, file_desc client_fifo, char *client_fifo_name) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATUS, getpid(), "", "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    int read_bytes_status, read_bytes_size, written_bytes, buffer_size;

    read_bytes_size = read(client_fifo, &buffer_size, sizeof(int));
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_size == -1, "Error reading from fifo")

    char *status = calloc(buffer_size, sizeof(char));

    read_bytes_status = read(client_fifo, status, buffer_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo")

    written_bytes = write(STDOUT_FILENO, status, buffer_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout");

    free(status);
    return 0;
}

int execute_stats_time(file_desc fifo, file_desc client_fifo, char *client_fifo_name, char *pids) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATS_TIME, getpid(), pids, "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    char result[2048];

    int read_bytes_status = read(client_fifo, result, sizeof(result));
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo")

    int written_bytes = write(STDOUT_FILENO, result, strlen(result));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout");
    
    return 0;
}

int execute_stats_command(file_desc fifo, file_desc client_fifo, char *client_fifo_name, char *info) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATS_COMMAND, getpid(), info, "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    char result[2048];

    int read_bytes_status = read(client_fifo, result, sizeof(result));
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo")

    int written_bytes = write(STDOUT_FILENO, result, strlen(result));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout");

    return 0;
}

int execute_stats_uniq(file_desc fifo, file_desc client_fifo, char *client_fifo_name, char *pids) {
    int ret_val = send_request_and_wait_notification(REQUEST_STATS_UNIQ, getpid(), pids, "", 0, client_fifo_name, fifo, client_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending the request.")

    char result[2048];

    int read_bytes_status = read(client_fifo, result, sizeof(result));
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes_status == -1, "Error reading from fifo")

    int written_bytes = write(STDOUT_FILENO, result, strlen(result));
    THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stdout");

    return 0;
}
