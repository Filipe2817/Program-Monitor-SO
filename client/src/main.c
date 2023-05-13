#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../include/executor.h"
#include "../../common/include/utils.h"
#include "../../common/include/request.h"
#include <stdbool.h>

#define FP

#ifdef FP
#define FIFO_NAME "/home/fp/fifos/Tracer-Monitor"
#else
#define FIFO_NAME "Tracer-Monitor"
#endif

void get_client_fifo_name(char *client_fifo_name) {
#ifdef FP
    sprintf(client_fifo_name, "/home/fp/fifos/client-%d", getpid());
#else
    sprintf(client_fifo_name, "client-%d", getpid());
#endif
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        ssize_t written_bytes = write(STDERR_FILENO, "Invalid Arguments!\n", 20);
        THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stderr\n");
        exit(EXIT_FAILURE);
    }

    file_desc fifo = open(FIFO_NAME, O_WRONLY);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    char client_fifo_name[32];
    get_client_fifo_name(client_fifo_name);

    int fifo_return = create_new_fifo(client_fifo_name);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo_return == -1, "Error creating FIFO\n");

    file_desc client_fifo = open(client_fifo_name, O_RDWR);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    if (!strcmp(argv[1], "execute") && (!strcmp(argv[2], "-u") || !strcmp(argv[2], "-p"))) {
        int ret_val = execute(argv[3], fifo, client_fifo, client_fifo_name);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error executing command\n");
    } else if (!strcmp(argv[1], "status")) {
        int ret_val = execute_status(fifo, client_fifo, client_fifo_name);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error getting status.\n");
    } else if (!strcmp(argv[1], "stats-time")) {
        char *pids = malloc(512 * sizeof(char)), *ptr = pids;
        for (int i = 2; i < argc; i++) {
            ptr = strnconcat(ptr, argv[i], strlen(argv[i]));
            ptr = strnconcat(ptr, " ", 1);
        }
        pids[strlen(pids) - 1] = '\0';
        int ret_val = execute_stats_time(fifo, client_fifo, client_fifo_name, pids);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error getting stats-time.\n");
        free(pids);
    } else if (!strcmp(argv[1], "stats-command")) {
        char *info = malloc(512 * sizeof(char)), *ptr = info;
        for (int i = 2; i < argc; i++) {
            ptr = strnconcat(ptr, argv[i], strlen(argv[i]));
            ptr = strnconcat(ptr, " ", 1);
        }
        info[strlen(info) - 1] = '\0';
        int ret_val = execute_stats_command(fifo, client_fifo, client_fifo_name, info);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error getting stats-command.\n");
        free(info);
    } else if (!strcmp(argv[1], "stats-uniq")) {
        char *pids = malloc(512 * sizeof(char)), *ptr = pids;
        for (int i = 2; i < argc; i++) {
            ptr = strnconcat(ptr, argv[i], strlen(argv[i]));
            ptr = strnconcat(ptr, " ", 1);
        }
        pids[strlen(pids) - 1] = '\0';
        int ret_val = execute_stats_uniq(fifo, client_fifo, client_fifo_name, pids);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error getting stats-uniq.\n");
        free(pids);
    }

    close(client_fifo);
    close(fifo);
    return 0;
}
