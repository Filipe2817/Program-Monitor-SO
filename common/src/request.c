#include <unistd.h>
#include <time.h>
#include "../include/request.h"

typedef enum request_type {
    REQUEST_EXECUTE,
    REQUEST_STATUS
} REQUEST_TYPE;

typedef struct request {
    REQUEST_TYPE type;
    pid_t pid;
    char *payload;
    int payload_size;
    struct timespec timestamp;
    char *response_fifo_name;
} Request;

Request *create_request(REQUEST_TYPE type, pid_t pid, char *payload, int payload_size, char *response_fifo_name) {
    Request *request = malloc(sizeof(Request));
    request->type = type;
    request->pid = pid;
    request->payload = payload;
    request->payload_size = payload_size;
    clock_gettime(CLOCK_REALTIME, &request->timestamp);
    request->response_fifo_name = response_fifo_name;
    return request;
}

void delete_request(Request *request) {
    free(request->payload);
    free(request->response_fifo_name);
    free(request);
}
