#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
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

    Hashtable *ht = create_hashtable();

    pid_t ongoing[100];
    int ongoing_index = 0;

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1)
    {
        Request *request = malloc(sizeof(struct request));
        THROW_ERROR_IF_FAILED_WITH_RETURN(request == NULL, "Error allocating memory\n");

        int read_bytes = receive_request(request, fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes != request->request_total_size, "Error receiving request\n");

        // printf("Received request:\n");
        // printf("Type: %d\n", request->type);
        // printf("PID: %d\n", request->pid);
        // printf("Program: %s\n", request->program);
        // printf("Timestamp: %s\n", request->timestamp);
        // printf("Execution time: %ld\n", request->execution_time);
        // printf("Response FIFO name: %s\n", request->response_fifo_name);
        // printf("Request total size: %d\n", request->request_total_size);

        file_desc response_fifo = open(request->response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");
        int ret_val = notify_sender(sizeof(struct request), response_fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error notifying sender\n");
        close(response_fifo);

        switch (request->type)
        {
        case REQUEST_EXECUTE_START:
            insert(ht, request->pid, request);

            ongoing[ongoing_index++] = request->pid;
            break;
        case REQUEST_EXECUTE_END:
            delete (ht, request->pid);
            insert(ht, request->pid, request);

            for (int i = 0; i < ongoing_index; i++)
            {
                if (ongoing[i] == request->pid)
                {
                    ongoing[i] = 1;
                }
            }

            print_ht(ht);
            break;
        case REQUEST_STATUS:
        {
            char status[1000];
            status[0] = '\0';

            for (int i = 0; i < ongoing_index; i++)
            {
                if (ongoing[i] != 1)
                {
                    struct timespec current;
                    clock_gettime(CLOCK_MONOTONIC, &current);
                    int elapsed_time = round((current.tv_sec - start.tv_sec) * 1000.0 + (current.tv_nsec - start.tv_nsec) / 1000000.0);

                    Request *req = get_request(ht, ongoing[i]);

                    char *status_line = malloc(100 * sizeof(char));
                    sprintf(status_line, "%d %s %d ms\n", req->pid, req->program, elapsed_time);
                    strcat(status, status_line);

                    // free(status);
                }
            }

            if (strlen(status) > 0)
            {

                status[strlen(status) - 1] = '\0';

                //printf("%s\n", status);

                file_desc status_fifo = open(request->response_fifo_name, O_WRONLY);
                THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");

                int written_bytes = write(status_fifo, status, strlen(status));
                THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to fifo.\n");

                close(status_fifo);
            }
            break;
        }
        default:
            break;
        }
    }

    // close(log);
    close(fifo);
    unlink(FIFO_NAME);
    free_ht(ht);
    return 0;
}
