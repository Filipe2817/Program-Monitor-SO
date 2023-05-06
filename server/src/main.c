#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "../../common/include/request.h"
#include "../../common/include/utils.h"

#define FP

#ifdef FP
#define FIFO_NAME "/home/fp/fifos/Tracer-Monitor"
#else
#define FIFO_NAME "Tracer-Monitor"
#endif

int main() {
    int fifo_return = createNewFifo(FIFO_NAME);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo_return == -1, "Error creating FIFO\n");

    file_desc fifo = open(FIFO_NAME, O_RDWR);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    while (1) {
        Request *request = malloc(sizeof(struct request));
        THROW_ERROR_IF_FAILED_WITH_RETURN(request == NULL, "Error allocating memory\n");
        
        int read_bytes = receive_request(request, fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes != request->request_total_size, "Error receiving request\n");

        printf("Received request:\n");
        printf("Type: %d\n", request->type);
        printf("PID: %d\n", request->pid);
        printf("Program: %s\n", request->program);
        printf("Timestamp: %s\n", request->timestamp);
        printf("Execution time: %ld\n", request->execution_time);
        printf("Response FIFO name: %s\n", request->response_fifo_name);
        printf("Request total size: %d\n", request->request_total_size);

        file_desc response_fifo = open(request->response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");
        int ret_val = notify_sender(sizeof(struct request), response_fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error notifying sender\n");

        close(response_fifo);
        free(request);
    }

    close(fifo);
    unlink(FIFO_NAME);
    return 0;
}
