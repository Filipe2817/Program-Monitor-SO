#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "../../common/include/request.h"
#include "../../common/include/utils.h"
#include "../../common/include/hashtable.h"
#include "../include/handler.h"

#define FIFO_NAME "Tracer-Monitor"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <storage directory>\n", argv[0]);
        exit(1);
    }

    int fifo_return = create_new_fifo(FIFO_NAME);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo_return == -1, "Error creating FIFO\n");

    file_desc fifo = open(FIFO_NAME, O_RDWR);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    Hashtable *finished_ht = create_hashtable();
    Hashtable *ongoing_ht = create_hashtable();

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        Request *request = malloc(sizeof(struct request));
        THROW_ERROR_IF_FAILED_WITH_RETURN(request == NULL, "Error allocating memory\n");

        int read_bytes = receive_request(request, fifo);

        file_desc response_fifo = open(request->response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");
        int ret_val = notify_sender(sizeof(struct request), response_fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error notifying sender\n");
        close(response_fifo);

        handle_request(request, ongoing_ht, finished_ht, argv[1]);
    }

    close(fifo);
    unlink(FIFO_NAME);
    free_ht(finished_ht);
    free_ht(ongoing_ht);
    return 0;
}
