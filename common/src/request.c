#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "../include/request.h"
#include "../include/utils.h"

Request *create_request(REQUEST_TYPE type, pid_t pid, char *program, char *timestamp, long execution_time, char *response_fifo_name)
{
    Request *request = malloc(sizeof(struct request));
    if (request == NULL)
        return NULL;
    request->type = type;
    request->pid = pid;
    request->program_size = strlen(program) + 1; // Assume null-terminate is part of the string
    request->program = strdup(program);
    request->timestamp_size = strlen(timestamp) + 1;
    request->timestamp = strdup(timestamp);
    request->execution_time = execution_time;
    request->response_fifo_name_size = strlen(response_fifo_name) + 1;
    request->response_fifo_name = strdup(response_fifo_name);
    request->request_total_size = sizeof(REQUEST_TYPE) + sizeof(pid_t) + sizeof(int) + request->program_size + sizeof(int) + request->timestamp_size + sizeof(long) + sizeof(int) + request->response_fifo_name_size + sizeof(int);
    return request;
}

int send_request(Request *request, file_desc fifo)
{
    int bytes_written = 0;

    /*
    Chars are bytes so we can serialize the struct in binary
    Request request;
    char *buf = (char *) &request;
    */

    char *buf = request_to_bytes(request, &bytes_written);
    THROW_ERROR_IF_FAILED_WITH_RETURN(buf == NULL, "Error serializing request\n");

    int ret_val = write(fifo, buf, bytes_written);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");

    free(buf);
    return 0;
}

int receive_request(Request *request, file_desc fifo)
{
    int bytes_read = 0;

    int ret_val = read(fifo, &request->type, sizeof(REQUEST_TYPE));
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    ret_val = read(fifo, &request->pid, sizeof(pid_t));
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    ret_val = read(fifo, &request->program_size, sizeof(int));
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    request->program = malloc(request->program_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(request->program == NULL, "Error allocating memory\n");
    ret_val = read(fifo, request->program, request->program_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    ret_val = read(fifo, &request->timestamp_size, sizeof(int));
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    request->timestamp = malloc(request->timestamp_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(request->timestamp == NULL, "Error allocating memory\n");
    ret_val = read(fifo, request->timestamp, request->timestamp_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    ret_val = read(fifo, &request->execution_time, sizeof(long));
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    ret_val = read(fifo, &request->response_fifo_name_size, sizeof(int));
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    request->response_fifo_name = malloc(request->response_fifo_name_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(request->response_fifo_name == NULL, "Error allocating memory\n");
    ret_val = read(fifo, request->response_fifo_name, request->response_fifo_name_size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    ret_val = read(fifo, &request->request_total_size, sizeof(int));
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");
    bytes_read += ret_val;

    return bytes_read;
}

int notify_sender(int received_bytes, file_desc fifo)
{
    if (received_bytes != sizeof(struct request))
    { // should never happen
        int ret_val = write(fifo, "ER", 2);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
        return 0;
    }

    int ret_val = write(fifo, "OK", 2);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
    return 0;
}

int wait_notification(file_desc fifo)
{
    char buffer[2];
    int ret_val = read(fifo, buffer, 2);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from FIFO\n");

    if (!strncmp(buffer, "OK", 2))
        return 0;

    if (!strncmp(buffer, "ER", 2))
        return 1;

    return -1;
}

int send_request_and_wait_notification(REQUEST_TYPE type, pid_t pid, char *program, char *timestamp, long execution_time, char *response_fifo_name, file_desc fifo, file_desc response_fifo)
{
    Request *request = create_request(type, pid, program, timestamp, execution_time, response_fifo_name);
    int ret_val = send_request(request, fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error sending request\n");
    delete_request(request);
    ret_val = wait_notification(response_fifo);
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error receiving notification\n");
    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == 1, "Request not received\n");
    return 0;
}

char *request_to_bytes(Request *request, int *size)
{
    if (request == NULL || size == NULL)
        return NULL;

    char *bytes = malloc(1024 * sizeof(char));
    if (bytes == NULL)
        return NULL;

    char *ptr = bytes;

    memcpy(ptr, &request->type, sizeof(REQUEST_TYPE));
    ptr += sizeof(REQUEST_TYPE);

    memcpy(ptr, &request->pid, sizeof(pid_t));
    ptr += sizeof(pid_t);

    memcpy(ptr, &request->program_size, sizeof(int));
    ptr += sizeof(int);

    memcpy(ptr, request->program, request->program_size);
    ptr += request->program_size;

    memcpy(ptr, &request->timestamp_size, sizeof(int));
    ptr += sizeof(int);

    memcpy(ptr, request->timestamp, request->timestamp_size);
    ptr += request->timestamp_size;

    memcpy(ptr, &request->execution_time, sizeof(long));
    ptr += sizeof(long);

    memcpy(ptr, &request->response_fifo_name_size, sizeof(int));
    ptr += sizeof(int);

    memcpy(ptr, request->response_fifo_name, request->response_fifo_name_size);
    ptr += request->response_fifo_name_size;

    memcpy(ptr, &request->request_total_size, sizeof(int));
    ptr += sizeof(int);

    *size = ptr - bytes;

    if (*size != request->request_total_size)
    {
        free(bytes);
        return NULL;
    }

    return bytes;
}

char *get_request_string(Request *request)
{
    char *request_string = malloc(1000 * sizeof(char));

    sprintf(request_string, "Type: %d\nPID: %d\nProgram size: %d\nProgram: %s\nTimestamp size: %d\nTimestamp: %s\nExecution time: %ld\nResponse FIFO name size: %d\nResponse FIFO name: %s\nRequest total size: %d", request->type, request->pid, request->program_size, request->program, request->timestamp_size, request->timestamp, request->execution_time, request->response_fifo_name_size, request->response_fifo_name, request->request_total_size);

    return request_string;
}

void print_request(Request *request)
{ // debug purposes
    printf("==================================== Received request ====================================\n");
    printf("Type: %d\n", request->type);
    printf("PID: %d\n", request->pid);
    printf("Program size: %d\n", request->program_size);
    printf("Program: %s\n", request->program);
    printf("Timestamp size: %d\n", request->timestamp_size);
    printf("Timestamp: %s\n", request->timestamp);
    printf("Execution time: %ld\n", request->execution_time);
    printf("Response FIFO name size: %d\n", request->response_fifo_name_size);
    printf("Response FIFO name: %s\n", request->response_fifo_name);
    printf("Request total size: %d\n", request->request_total_size);
    printf("==========================================================================================\n\n");
}

void delete_request(Request *request)
{
    free(request->program);
    free(request->timestamp);
    free(request->response_fifo_name);
    free(request);
}
