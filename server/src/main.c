#include <stdio.h>
#include <fcntl.h>
#include <string.h>
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

        printf("==================== Received request ====================\n");
        //x = write(STDOUT_FILENO, request->type, sizeof(request->type));
        //int x = write(STDOUT_FILENO, "\nPID: ", 6);
        //x = write(STDOUT_FILENO, request->pid, sizeof(request->pid));
        int x = write(STDOUT_FILENO, "\nProgram size: ", 15);
        x = write(STDOUT_FILENO, &request->program_size, sizeof(request->program_size));
        x = write(STDOUT_FILENO, "\nProgram: ", 10);
        x = write(STDOUT_FILENO, request->program, request->program_size);
        x = write(STDOUT_FILENO, "\nTimestamp size: ", 17);
        x = write(STDOUT_FILENO, &request->timestamp_size, sizeof(request->timestamp_size));
        x = write(STDOUT_FILENO, "\nTimestamp: ", 12);
        x = write(STDOUT_FILENO, request->timestamp, request->timestamp_size);
        x = write(STDOUT_FILENO, "\nExecution time: ", 17);
        x = write(STDOUT_FILENO, &request->execution_time, sizeof(request->execution_time));
        x = write(STDOUT_FILENO, "\nResponse FIFO name size: ", 26);
        x = write(STDOUT_FILENO, &request->response_fifo_name_size, sizeof(request->response_fifo_name_size));
        x = write(STDOUT_FILENO, "\nResponse FIFO name: ", 21);
        x = write(STDOUT_FILENO, request->response_fifo_name, request->response_fifo_name_size);
        x = write(STDOUT_FILENO, "\nRequest total size: ", 21);
        x = write(STDOUT_FILENO, &request->request_total_size, sizeof(request->request_total_size));
        x = write(STDOUT_FILENO, "\n", 1);
        printf("===========================================================\n\n");

        file_desc response_fifo = open(request->response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");
        int ret_val = notify_sender(sizeof(struct request), response_fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error notifying sender\n");

        close(response_fifo);
        delete_request(request);
    }

    close(fifo);
    unlink(FIFO_NAME);
    return 0;
}
