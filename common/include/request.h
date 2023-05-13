#ifndef REQUEST_H
#define REQUEST_H

#include "utils.h"
#include <stdbool.h>

typedef enum request_type {
    REQUEST_EXECUTE_START,
    REQUEST_EXECUTE_END,
    REQUEST_STATUS,
    REQUEST_STATS_TIME,
    REQUEST_STATS_COMMAND,
    REQUEST_STATS_UNIQ
} REQUEST_TYPE;

typedef struct request {
    REQUEST_TYPE type;
    pid_t pid;
    int payload_size;
    char *payload;
    int timestamp_size;
    char *timestamp;
    long execution_time;
    int response_fifo_name_size;
    char *response_fifo_name;
    int request_total_size;
} Request;

Request *create_request(REQUEST_TYPE type, pid_t pid, char *payload, char *timestamp, long execution_time, char *response_fifo_name);

int send_request(Request *request, file_desc fifo);

int receive_request(Request *request, file_desc fifo);

int notify_sender(int received_bytes, file_desc fifo);

int wait_notification(file_desc fifo);

int send_request_and_wait_notification(REQUEST_TYPE type, pid_t pid, char *payload, char *timestamp, long execution_time, char *response_fifo_name, file_desc fifo, file_desc response_fifo);

char *request_to_bytes(Request *request, int *size);

char *get_request_string(Request *request);

void print_request(Request *request);

Request *read_request_from_file(char *path);

void delete_request(Request *request);

#endif