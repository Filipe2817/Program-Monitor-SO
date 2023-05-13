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
#include "../../common/include/array.h"

#define FP

#ifdef FP
#define FIFO_NAME "/home/fp/fifos/Tracer-Monitor"
#else
#define FIFO_NAME "Tracer-Monitor"
#endif

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
        THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes != request->request_total_size, "Error receiving request\n");

        //print_request(request);

        file_desc response_fifo = open(request->response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");
        int ret_val = notify_sender(sizeof(struct request), response_fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error notifying sender\n");
        close(response_fifo);

        switch (request->type) {
        case REQUEST_EXECUTE_START:
            insert(ongoing_ht, request->pid, request);
            break;

        case REQUEST_EXECUTE_END:
            delete (ongoing_ht, request->pid);
            insert(finished_ht, request->pid, request);

            char *request_string = get_request_string(request);

            char path[32];
            snprintf(path, 32, "%s/%d.txt", argv[1], request->pid);

            int storage_fd = open(path, O_WRONLY | O_CREAT, 0666);
            THROW_ERROR_IF_FAILED_WITH_RETURN(storage_fd == -1, "Error opening file.\n");

            int ret_val = write(storage_fd, request_string, strlen(request_string));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to file\n");

            close(storage_fd);
            free(request_string);
            // print_ht(ht);
            break;

        case REQUEST_STATUS: ;
            char *status = get_ongoing_programs(ongoing_ht);

            file_desc status_fifo = open(request->response_fifo_name, O_WRONLY);
            THROW_ERROR_IF_FAILED_WITH_RETURN(status_fifo == -1, "Error opening response FIFO.\n");

            int status_size = strlen(status);
            int ret_val_size = write(status_fifo, &status_size, sizeof(status_size));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val_size == -1, "Error writing to FIFO\n");

            int ret_val_status = write(status_fifo, status, strlen(status));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val_status == -1, "Error writing to FIFO\n");

            free(status);
            close(status_fifo);
            break;

        case REQUEST_STATS_TIME:
            if (fork() == 0) {
                char **info = malloc(64 * sizeof(char *));
                int num_pids = parse_command(request->payload, info, " ");
                Array *files = get_file_pids(argv[1]);

                int *pids = malloc(num_pids * sizeof(int));
                convert_string_array_to_int_array(info, pids, num_pids);
                free(info[0]);
                free(info);

                // sort to avoid multiple loops over the directory
                qsort(pids, num_pids, sizeof(int), compare_ints);
                sort_array(files);

                int file_index = 0, pid_index = 0;
                long total_time = 0;

                while (pid_index < num_pids && file_index < files->size) {
                    if (files->array[file_index] < pids[pid_index]) {
                        file_index++;
                        continue;
                    }

                    if (files->array[file_index] > pids[pid_index]) {
                        pid_index++;
                        continue;
                    }

                    if (files->array[file_index] == pids[pid_index]) {
                        char file_path[32];
                        snprintf(file_path, sizeof(file_path), "%s/%d.txt", argv[1], pids[pid_index]);

                        Request *request = read_request_from_file(file_path);
                        total_time += request->execution_time;
                        delete_request(request);

                        pid_index++;
                        file_index++;
                    }
                }
                free(pids);
                delete_array(files);

                char *buf = malloc(64 * sizeof(char));
                sprintf(buf, "Total execution time is %ld\n", total_time);
                file_desc fifo = open(request->response_fifo_name, O_WRONLY);
                int ret_val = write(fifo, buf, strlen(buf) + 1);
                THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
                close(fifo);
                free(buf);
            }
            break;

        case REQUEST_STATS_COMMAND:
            if (fork() == 0) {
                char **info = malloc(64 * sizeof(char *));
                int num_pids = parse_command(request->payload, info, " ") - 1;
                Array *files = get_file_pids(argv[1]);

                char *command = strdup(info[0]);
                int *pids = malloc(num_pids * sizeof(int));
                convert_string_array_to_int_array(info + 1, pids, num_pids);
                free(info[0]);
                free(info);

                // sort to avoid multiple loops over the directory
                qsort(info, num_pids, sizeof(int), compare_ints);
                sort_array(files);

                int file_index = 0, pid_index = 0, total_executions = 0;

                while (pid_index < num_pids && file_index < files->size) {
                    if (files->array[file_index] < pids[pid_index]) {
                        file_index++;
                        continue;
                    }

                    if (files->array[file_index] > pids[pid_index]) {
                        pid_index++;
                        continue;
                    }

                    if (files->array[file_index] == pids[pid_index]) {
                        char file_path[32];
                        snprintf(file_path, sizeof(file_path), "%s/%d.txt", argv[1], pids[pid_index]);

                        Request *request = read_request_from_file(file_path);
                        if (!strcmp(request->payload, command)) {
                            total_executions++;
                        }
                        delete_request(request);

                        pid_index++;
                        file_index++;
                    }
                }
                free(pids);
                delete_array(files);

                char *buf = malloc(64 * sizeof(char));
                sprintf(buf, "%s was executed %d times\n", command, total_executions);
                file_desc fifo = open(request->response_fifo_name, O_WRONLY);
                int ret_val = write(fifo, buf, strlen(buf) + 1);
                THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
                close(fifo);
                free(command);
                free(buf);
            }
            break;

        case REQUEST_STATS_UNIQ: ;
            if (fork() == 0) {
                char **info = malloc(64 * sizeof(char *));
                int num_pids = parse_command(request->payload, info, " ");
                Array *files = get_file_pids(argv[1]);

                int *pids = malloc(num_pids * sizeof(int));
                convert_string_array_to_int_array(info, pids, num_pids);
                free(info[0]);
                free(info);

                // sort to avoid multiple loops over the directory
                qsort(pids, num_pids, sizeof(int), compare_ints);
                sort_array(files);

                int file_index = 0, pid_index = 0, commands_size = 0;
                char **unique_commands = malloc(32 * sizeof(char *));

                while (pid_index < num_pids && file_index < files->size) {
                    if (files->array[file_index] < pids[pid_index]) {
                        file_index++;
                        continue;
                    }

                    if (files->array[file_index] > pids[pid_index]) {
                        pid_index++;
                        continue;
                    }

                    if (files->array[file_index] == pids[pid_index]) {
                        char file_path[32];
                        snprintf(file_path, sizeof(file_path), "%s/%d.txt", argv[1], pids[pid_index]);

                        Request *request = read_request_from_file(file_path);
                        if (!is_in_array(unique_commands, request->payload, commands_size)) {
                            unique_commands[commands_size] = malloc(64 * sizeof(char));
                            strcpy(unique_commands[commands_size++], request->payload);
                        }
                        delete_request(request);

                        pid_index++;
                        file_index++;
                    }
                }
                free(pids);
                delete_array(files);

                char *buf = malloc(256 * sizeof(char)), *ptr = buf;

                for (int i = 0; i < commands_size; i++) {
                    ptr = strnconcat(ptr, unique_commands[i], strlen(unique_commands[i]));
                    ptr = strnconcat(ptr, "\n", 1);
                }
                buf[strlen(buf)] = '\0';

                file_desc fifo = open(request->response_fifo_name, O_WRONLY);
                int ret = write(fifo, buf, strlen(buf) + 1);
                THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");
                close(fifo);
                for (int i = 0; i < commands_size; i++) {
                    free(unique_commands[i]);
                }
                free(unique_commands);
                free(buf);
            }
            break;

        default:
            printf("Unknown Request Type\n");
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
