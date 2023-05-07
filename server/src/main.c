#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include "../../common/include/request.h"
#include "../../common/include/utils.h"
#include "../../common/include/request.h"
#include "../../common/include/hashtable.h"

// #define FP

#ifdef FP
#define FIFO_NAME "/home/fp/fifos/Tracer-Monitor"
#else
#define FIFO_NAME "Tracer-Monitor"
#endif

int main(int argc, char *argv[])
{
    int fifo_return = createNewFifo(FIFO_NAME);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo_return == -1, "Error creating FIFO\n");

    file_desc fifo = open(FIFO_NAME, O_RDWR);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    Hashtable *finished_ht = create_hashtable();
    Hashtable *ongoing_ht = create_hashtable();

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1)
    {
        Request *request = malloc(sizeof(struct request));
        THROW_ERROR_IF_FAILED_WITH_RETURN(request == NULL, "Error allocating memory\n");

        int read_bytes = receive_request(request, fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes != request->request_total_size, "Error receiving request\n");

        // print_request(request);

        file_desc response_fifo = open(request->response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");
        int ret_val = notify_sender(sizeof(struct request), response_fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error notifying sender\n");
        close(response_fifo);

        switch (request->type)
        {
        case REQUEST_EXECUTE_START:
            insert(ongoing_ht, request->pid, request);

            break;
        case REQUEST_EXECUTE_END:
            delete (ongoing_ht, request->pid);
            insert(finished_ht, request->pid, request);

            char *request_string = get_request_string(request);

            char path[100];

            if (argc == 2)
            {
                snprintf(path, 100, "%s/%d.txt", argv[1], request->pid);
            }
            else
            {
                snprintf(path, 100, "%d.txt", request->pid);
            }

            int storage_fd = open(path, O_WRONLY | O_CREAT, 0666);
            THROW_ERROR_IF_FAILED_WITH_RETURN(storage_fd == -1, "Error opening file.\n");

            int ret_val = write(storage_fd, request_string, strlen(request_string));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to file\n");

            close(storage_fd);
            free(request_string);
            // print_ht(ht);
            break;
        case REQUEST_STATUS:
        {
            char *status = get_ongoing_programs(ongoing_ht);

            file_desc status_fifo = open(request->response_fifo_name, O_WRONLY);
            THROW_ERROR_IF_FAILED_WITH_RETURN(status_fifo == -1, "Error opening response FIFO.\n");

            int status_size = strlen(status);
            int ret_val_size = write(status_fifo, &status_size, sizeof(status_size));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val_size == -1, "Error writing to FIFO\n");

            int ret_val_status = write(status_fifo, status, strlen(status));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val_status == -1, "Error writing to FIFO\n");

            free(status);
            close(status_fifo);

            break;
        }
        default:
            break;
        }
    }

    // close(log);
    close(fifo);
    unlink(FIFO_NAME);
    free_ht(finished_ht);
    free_ht(ongoing_ht);
    return 0;
}
