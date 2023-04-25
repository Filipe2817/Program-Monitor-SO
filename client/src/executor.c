#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

int executor(const char *command) {
    char *cmd_copy = strdup(command), *token = NULL, *saveptr = NULL;
    char **args = calloc(256, sizeof(char *));
    int argc = 0;

    saveptr = cmd_copy;

    while ((token = strsep(&cmd_copy, " ")) != NULL) {
        if (*token == '\0')
            continue; // Skip empty tokens
        args[argc++] = token;
    }

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
        free(saveptr);
        free(args);
        perror("Fork failed");
        return -1;
    }

    char buf[24];
    snprintf(buf, sizeof(buf), "Running PID %d\n", pid);
    int written_bytes = write(STDOUT_FILENO, buf, strlen(buf));
    if (written_bytes == -1) {
        perror("Error writing to stdout");
        free(saveptr);
        free(args);
        return -1;
    }

    int status;
    waitpid(pid, &status, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    int elapsed_time = round((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0);

    int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    if (exit_status == -1) {
        //printf("Error executing program %s\n", args[0]);
        write(STDERR_FILENO, "Error executing program\n", 24);
        free(saveptr);
        free(args);
        return -1;
    }

    char time_buf[24];
    snprintf(time_buf, sizeof(time_buf), "Ended in %d ms\n", elapsed_time);
    written_bytes = write(STDOUT_FILENO, time_buf, strlen(time_buf));
    if (written_bytes == -1) {
        perror("Error writing to stdout");
        free(saveptr);
        free(args);
        return -1;
    }

    //printf("Program %s exited with code %d\n", args[0], exit_status);
    free(saveptr);
    free(args);
    return exit_status;
}
