#include <unistd.h>
#include <string.h>
#include <time.h>
#include "../include/request.h"
#include "../include/utils.h"

Request *create_request(REQUEST_TYPE type, pid_t pid, char *program, char *timestamp, long execution_time, char *response_fifo_name) {
    Request *request = malloc(sizeof(struct request));
    if (request == NULL)
        return NULL;
    request->type = type;
    request->pid = pid;
    request->program = strdup(program);
    request->program_size = strlen(program);
    request->timestamp = strdup(timestamp);
    request->timestamp_size = strlen(timestamp);
    request->execution_time = execution_time;
    request->response_fifo_name = response_fifo_name;
    return request;
}

int send_request(Request *request, file_desc fifo) {
    int bytes_written = 0, total_bytes = sizeof(struct request);

    /*
    Chars are bytes so we can serialize the struct in binary
    Request request;
    char *buf = (char *) &request;
    */

    while (bytes_written < total_bytes) {
        int ret_val = write(fifo, ((char *)request) + bytes_written, total_bytes - bytes_written);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
        bytes_written += ret_val;
    }

    return 0;
}

int receive_request(Request *request, file_desc fifo) {
    int bytes_read = 0, total_bytes = sizeof(struct request);

    while (bytes_read < total_bytes) {
        int ret_val = read(fifo, ((char *)request) + bytes_read, total_bytes - bytes_read);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
        bytes_read += ret_val;
    }

    return 0;
}

int notify_sender(int received_bytes, file_desc fifo) {
    if (received_bytes != sizeof(struct request)) { // should never happen
        int ret_val = write(fifo, "ER", 2);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
        return 0;
    }

    int ret_val = write(fifo, "OK", 2);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
    return 0;
}

int wait_notification(file_desc fifo) {
    char buffer[2];
    int ret_val = read(fifo, buffer, 2);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    
    if (!strncmp(buffer, "OK", 2))
        return 0;
    
    if (!strncmp(buffer, "ER", 2))
        return 1;
    
    return -1;
}

int send_request_and_wait_notification(REQUEST_TYPE type, pid_t pid, char *program, char *timestamp, long execution_time, char *response_fifo_name, file_desc fifo, file_desc response_fifo) {
    Request *request = create_request(type, pid, program, timestamp, execution_time, response_fifo_name);
    int ret_val = send_request(request, fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending request\n");
    ret_val = wait_notification(response_fifo);
    delete_request(request);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error receiving notification\n");
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == 1, "Request not received\n");
    return 0;
}

void delete_request(Request *request) {
    free(request->program);
    free(request->timestamp);
    free(request->response_fifo_name);
    free(request);
}
