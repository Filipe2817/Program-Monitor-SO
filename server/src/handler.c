#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "../../common/include/request.h"
#include "../../common/include/utils.h"
#include "../../common/include/array.h"
#include "../../common/include/hashtable.h"
#include "../include/handler.h"

void handle_execute_start(Request *request, Hashtable *ongoing) {
    insert(ongoing, request->pid, request);
}

void handle_execute_end(Request *request, Hashtable *ongoing, Hashtable *finished, char *directory) {
    delete (ongoing, request->pid);
    insert(finished, request->pid, request);

    char *request_string = get_request_string(request);

    char path[32];
    snprintf(path, 32, "%s/%d.txt", directory, request->pid);

    int storage_fd = open(path, O_WRONLY | O_CREAT, 0666);
    THROW_ERROR_IF_FAILED_VOID(storage_fd == -1, "Error opening file.\n");

    int ret_val = write(storage_fd, request_string, strlen(request_string));
    THROW_ERROR_IF_FAILED_VOID(ret_val == -1, "Error writing to file\n");

    close(storage_fd);
    free(request_string);
}

void handle_status(Hashtable *ongoing, char *response_fifo_name) {
    if (fork() == 0) {
        char *status = get_ongoing_programs(ongoing);

        file_desc status_fifo = open(response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_VOID(status_fifo == -1, "Error opening response FIFO.\n");

        int status_size = strlen(status);
        int ret_val_size = write(status_fifo, &status_size, sizeof(status_size));
        THROW_ERROR_IF_FAILED_VOID(ret_val_size == -1, "Error writing to FIFO\n");

        int ret_val_status = write(status_fifo, status, strlen(status));
        THROW_ERROR_IF_FAILED_VOID(ret_val_status == -1, "Error writing to FIFO\n");

        free(status);
        close(status_fifo);
    }
}

void handle_stats_time(char *payload, char *directory, char *response_fifo_name) {
    if (fork() == 0) {
        char **info = malloc(64 * sizeof(char *));
        int num_pids = parse_command(payload, info, " ");
        Array *files = get_file_pids(directory);
        int *pids = NULL;

        if (num_pids > 0) {
            pids = malloc(num_pids * sizeof(int));
        } else {
            free(info[0]);
            free(info);
            delete_array(files);
            file_desc fifo = open(response_fifo_name, O_WRONLY);
            int ret_val = write(fifo, "Total execution time is 0\n", 27);
            THROW_ERROR_IF_FAILED_VOID(ret_val == -1, "Error writing to FIFO\n");
            close(fifo);
        }

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
                snprintf(file_path, sizeof(file_path), "%s/%d.txt", directory, pids[pid_index]);

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
        file_desc fifo = open(response_fifo_name, O_WRONLY);
        int ret_val = write(fifo, buf, strlen(buf) + 1);
        THROW_ERROR_IF_FAILED_VOID(ret_val == -1, "Error writing to FIFO\n");
        close(fifo);
        free(buf);
    }
}

void handle_stats_command(char *payload, char *directory, char *response_fifo_name) {
    if (fork() == 0) {
        char **info = malloc(64 * sizeof(char *));
        int num_pids = parse_command(payload, info, " ") - 1;
        Array *files = get_file_pids(directory);
        int *pids = NULL;
        char *command = NULL;

        if (num_pids > 1) {
            command = strdup(info[0]);
            pids = malloc(num_pids * sizeof(int));
        } else {
            free(info[0]);
            free(info);
            delete_array(files);
            file_desc fifo = open(response_fifo_name, O_WRONLY);
            int ret_val = write(fifo, "Wrong usage of stats command\n", 30);
            THROW_ERROR_IF_FAILED_VOID(ret_val == -1, "Error writing to FIFO\n");
            close(fifo);
        }

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
                snprintf(file_path, sizeof(file_path), "%s/%d.txt", directory, pids[pid_index]);

                Request *request = read_request_from_file(file_path);
                char **commands = malloc(32 * sizeof(char *));
                int num_commands = parse_command(request->payload, commands, " |");

                for (int i = 0; i < num_commands; i++) {
                    if (!strcmp(commands[i], command)) {
                        total_executions++;
                    }
                }

                free(commands[0]);
                free(commands);
                delete_request(request);
                pid_index++;
                file_index++;
            }
        }
        free(pids);
        delete_array(files);

        char *buf = malloc(64 * sizeof(char));
        sprintf(buf, "%s was executed %d times\n", command, total_executions);
        file_desc fifo = open(response_fifo_name, O_WRONLY);
        int ret_val = write(fifo, buf, strlen(buf) + 1);
        THROW_ERROR_IF_FAILED_VOID(ret_val == -1, "Error writing to FIFO\n");
        close(fifo);
        free(command);
        free(buf);
    }
}

void handle_stats_uniq(char *payload, char *directory, char *response_fifo_name) {
    if (fork() == 0) {
        char **info = malloc(64 * sizeof(char *));
        int num_pids = parse_command(payload, info, " ");
        Array *files = get_file_pids(directory);
        int *pids = NULL;

        if (num_pids > 0) {
            pids = malloc(num_pids * sizeof(int));
        } else {
            free(info[0]);
            free(info);
            delete_array(files);
            file_desc fifo = open(response_fifo_name, O_WRONLY);
            int ret_val = write(fifo, "No PIDS provided\n", 18);
            THROW_ERROR_IF_FAILED_VOID(ret_val == -1, "Error writing to FIFO\n");
            close(fifo);
        }

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
                snprintf(file_path, sizeof(file_path), "%s/%d.txt", directory, pids[pid_index]);

                Request *request = read_request_from_file(file_path);
                char **commands = malloc(32 * sizeof(char *));
                int num_commands = parse_command(request->payload, commands, " |");

                for (int i = 0; i < num_commands; i++) {
                    if (!is_in_array(unique_commands, commands[i], commands_size)) {
                        unique_commands[commands_size] = malloc(32 * sizeof(char));
                        strcpy(unique_commands[commands_size++], commands[i]);
                    }
                }

                free(commands[0]);
                free(commands);
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

        file_desc fifo = open(response_fifo_name, O_WRONLY);
        int ret_val = write(fifo, buf, strlen(buf) + 1);
        THROW_ERROR_IF_FAILED_VOID(ret_val == -1, "Error writing to FIFO\n");
        close(fifo);
        for (int i = 0; i < commands_size; i++) {
            free(unique_commands[i]);
        }
        free(unique_commands);
        free(buf);
    }
}

void handle_request(Request *request, Hashtable *ongoing_ht, Hashtable *finished_ht, char *directory) {
    switch (request->type) {
    case REQUEST_EXECUTE_START:
        handle_execute_start(request, ongoing_ht);
        break;

    case REQUEST_EXECUTE_END:
        handle_execute_end(request, ongoing_ht, finished_ht, directory);
        break;

    case REQUEST_STATUS:
        handle_status(ongoing_ht, request->response_fifo_name);
        break;

    case REQUEST_STATS_TIME:
        handle_stats_time(request->payload, directory, request->response_fifo_name);
        break;

    case REQUEST_STATS_COMMAND:
        handle_stats_command(request->payload, directory, request->response_fifo_name);
        break;

    case REQUEST_STATS_UNIQ:
        handle_stats_uniq(request->payload, directory, request->response_fifo_name);
        break;

    default:
        printf("Unknown Request Type\n");
        break;
    }
}
