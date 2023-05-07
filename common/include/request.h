#ifndef REQUEST_H
#define REQUEST_H

#include "utils.h"

typedef enum request_type {
    REQUEST_EXECUTE_START,
    REQUEST_EXECUTE_END,
    REQUEST_STATUS
} REQUEST_TYPE;

typedef struct request {
    REQUEST_TYPE type;
    pid_t pid;
    int program_size;
    char *program;
    int timestamp_size;
    char *timestamp;
    long execution_time;
    int response_fifo_name_size;
    char *response_fifo_name;
    int request_total_size;
} Request;

Request *create_request(REQUEST_TYPE type, pid_t pid, char *program, char *timestamp, long execution_time, char *response_fifo_name);

int send_request(Request *request, file_desc fifo);

int receive_request(Request *request, file_desc fifo);

int notify_sender(int received_bytes, file_desc fifo);

int wait_notification(file_desc fifo);

int send_request_and_wait_notification(REQUEST_TYPE type, pid_t pid, char *program, char *timestamp, long execution_time, char *response_fifo_name, file_desc fifo, file_desc response_fifo);

char *request_to_bytes(Request *request, int *size);

//char *request_to_byte(Request *req);

Request *byte_to_request(char *bytes);

void delete_request(Request *request);

#endif