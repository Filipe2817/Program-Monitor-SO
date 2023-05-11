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

void get_client_fifo_name(char *client_fifo_name)
{
#ifdef FP
    sprintf(client_fifo_name, "/home/fp/fifos/client-%d", getpid());
#else
    sprintf(client_fifo_name, "client-%d", getpid());
#endif
}

int main(int argc, char *argv[])
{
    // if (argc != 4)
    // {
    //     ssize_t written_bytes = write(STDERR_FILENO, "Invalid Arguments!\n Use: ./main-program execute -u \"prog arg-1 ... arg-n\"\n", 74);
    //     THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stderr\n");
    // }

    file_desc fifo = open(FIFO_NAME, O_WRONLY);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    char client_fifo_name[32];
    get_client_fifo_name(client_fifo_name);

    int fifo_return = createNewFifo(client_fifo_name);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo_return == -1, "Error creating FIFO\n");

    file_desc client_fifo = open(client_fifo_name, O_RDWR);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u"))
    {
        int ret_val = executor(argv[3], fifo, client_fifo, client_fifo_name);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error executing command\n");
    }
    else if (!strcmp(argv[1], "status"))
    {
        int ret_val = execute_status(fifo, client_fifo, client_fifo_name);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error getting status.\n");
    }
    else if (!strcmp(argv[1], "stats_time"))
    {
        char *pids = malloc(sizeof(char*));
        for (int i = 0; i < argc; i++) {
            strcat(pids, argv[i]);
            strcat(pids, " ");
        }
        int ret_val = execute_stats_time(fifo, client_fifo, client_fifo_name, pids);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error executing command\n");

        free(pids);
    }
    else if (!strcmp(argv[1], "stats_command"))
    {
        int ret_val = execute_stats_command(fifo, client_fifo, client_fifo_name, argv[2]);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error executing command\n");
    }
    else if (!strcmp(argv[1], "stats_uniq"))
    {
        int ret_val = execute_stats_uniq(fifo, client_fifo, client_fifo_name);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error executing command\n");
    }

    close(fifo);
    return 0;
}
