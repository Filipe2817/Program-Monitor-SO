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

int main()
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

        print_request(request);

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

            // print_ht(ht);
            break;
        case REQUEST_STATUS:
        {
            char *status = get_ongoing_programs(ongoing_ht, start);

            file_desc status_fifo = open(request->response_fifo_name, O_WRONLY);
            THROW_ERROR_IF_FAILED_WITH_RETURN(status_fifo == -1, "Error opening response FIFO.\n");

            int ret_val = write(status_fifo,status,strlen(status));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");

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
