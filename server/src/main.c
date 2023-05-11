#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include "../../common/include/request.h"
#include "../../common/include/utils.h"
#include "../../common/include/request.h"
#include "../../common/include/hashtable.h"
#include <stdbool.h>

// #define FP

#ifdef FP
#define FIFO_NAME "/home/fp/fifos/Tracer-Monitor"
#else
#define FIFO_NAME "Tracer-Monitor"
#endif

int main(int argc, char *argv[])
{
    int fifo_return = createNewFifo(FIFO_NAME);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo_return == -1, "Error creating FIFO\n");

    file_desc fifo = open(FIFO_NAME, O_RDWR);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    Hashtable *finished_ht = create_hashtable();
    Hashtable *ongoing_ht = create_hashtable();

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1)
    {
        Request *request = malloc(sizeof(struct request));
        THROW_ERROR_IF_FAILED_WITH_RETURN(request == NULL, "Error allocating memory\n");

        int read_bytes = receive_request(request, fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes != request->request_total_size, "Error receiving request\n");

        // print_request(request);

        file_desc response_fifo = open(request->response_fifo_name, O_WRONLY);
        THROW_ERROR_IF_FAILED_WITH_RETURN(response_fifo == -1, "Error opening response FIFO\n");
        int ret_val = notify_sender(sizeof(struct request), response_fifo);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error notifying sender\n");
        close(response_fifo);

        switch (request->type)
        {
        case REQUEST_EXECUTE_START:
            insert(ongoing_ht, request->pid, request);

            break;
        case REQUEST_EXECUTE_END:
            delete (ongoing_ht, request->pid);
            insert(finished_ht, request->pid, request);

            char *request_string = get_request_string(request);

            char path[100];

            if (argc == 2)
            {
                snprintf(path, 100, "%s/%d.txt", argv[1], request->pid);
            }
            else
            {
                snprintf(path, 100, "%d.txt", request->pid);
            }

            int storage_fd = open(path, O_WRONLY | O_CREAT, 0666);
            THROW_ERROR_IF_FAILED_WITH_RETURN(storage_fd == -1, "Error opening file.\n");

            int ret_val = write(storage_fd, request_string, strlen(request_string));
            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to file\n");

            close(storage_fd);
            free(request_string);
            // print_ht(ht);
            break;
        case REQUEST_STATUS:
        {
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
        }
        case REQUEST_STATS_TIME:
        {
            pid_t pid = fork();
            if (pid == 0)
            { 
                long final_value = 0;
                char** list = malloc(sizeof(char**));
                parse_command(request->program, list);
                DIR *d;
                struct dirent *dir;
                d = opendir(argv[1]);
                if (d) {
                    while ((dir = readdir(d)) != NULL) {
                        //printf("%s\n", dir->d_name);
                        if(found_in(list, dir->d_name)){
                            
                            file_desc fd = open(dir->d_name, O_RDONLY);
                            char* read_buf = malloc(sizeof(char*));
                            int ret_val = read(fd, read_buf, 500);
                            THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from File\n");
                            close(fd);

                            int count = 0;
                            int flag = 0;
                            char *paragraph = strtok(read_buf, "\n");
                            while (paragraph != NULL && flag == 0) {
                                count++;
                                if (count == 7)
                                    flag = 1;
                                if (flag == 0)
                                    paragraph = strtok(NULL, "\n");
                            }
                            char[strlen(paragraph) - 16] time;
                            strncpy(time, paragraph + 16, (strlen(paragraph) - 16));
                            time[strlen(paragraph) - 16] = 0;
                            
                            char *endptr;
                            long val = strtol(time, &endptr, 10);

                            final_value = final_value + val;
                            free(endptr);
                            free(paragraph);
                            free(read_buf);
                        }
                    }
                    closedir(d);

                    char *buf = malloc(sizeof(char*));
                    sprintf(buf, "Stats_time: %ld milisegundos", final_value);
                    file_desc fifo = open(request->response_fifo_name, WRONLY);
                    int ret_val = write(fifo, buf, strlen(buf));
                    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");

                    close(fifo);
                    free(buf);
                }

                int i = 0;
                while(list[i] != NULL){

                    free(list[i]);
                    i++;
                }
                free(list);
            }
            write(STDOUT_FILENO, "Request_Stats_Time Forked", 26);
        }
        case REQUEST_STATS_COMMAND:
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                int final_count = 0;
                DIR *d;
                struct dirent *dir;
                d = opendir(argv[1]);
                if (d) {
                    while ((dir = readdir(d)) != NULL) {
                        //printf("%s\n", dir->d_name);
                        file_desc fd = open(dir->d_name, O_RDONLY);
                        char* read_buf = malloc(sizeof(char*));
                        int ret_val = read(fd, read_buf, 500);
                        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from File\n");
                        close(fd);
                
                        int count = 0;
                        int flag = 0;
                        char *paragraph = strtok(read_buf, "\n");
                        while (paragraph != NULL && flag == 0) {
                            count++;
                            if (count == 4)
                                flag = 1;
                            if (flag == 0)
                                paragraph = strtok(NULL, "\n");
                        }
                        char[strlen(paragraph) - 9] prog;
                        strncpy(prog, paragraph + 9, (strlen(paragraph) - 9));
                        prog[strlen(paragraph) - 9] = 0;

                        if (strcmp(prog, request->program)){
                            final_count++;
                        }

                        free(paragraph);
                        free(read_buf);
                    }
                    closedir(d);

                    char *buf = malloc(sizeof(char*));
                    sprintf(buf, "Stats_command: %d vezes", final_count);
                    file_desc fifo = open(request->response_fifo_name, WRONLY);
                    int ret_val = write(fifo, buf, strlen(buf));
                    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");

                    close(fifo);
                    free(buf);
                }
            }
            write(STDOUT_FILENO, "Request_Stats_Command Forked", 26);
        }
        case REQUEST_STATS_UNIQ:
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                char **list = malloc(sizeof(char**));
                int i = 0;
                DIR *d;
                struct dirent *dir;
                d = opendir(argv[1]);
                if (d) {
                    while ((dir = readdir(d)) != NULL) {
                        //printf("%s\n", dir->d_name);
                        file_desc fd = open(dir->d_name, O_RDONLY);
                        char* read_buf = malloc(sizeof(char*));
                        int ret_val = read(fd, read_buf, 500);
                        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error reading from File\n");
                        close(fd);

                        int count = 0;
                        int flag = 0;
                        char *paragraph = strtok(read_buf, "\n");
                        while (paragraph != NULL && flag == 0) 
                        {
                            count++;
                            if (count == 4)
                                flag = 1;
                            if (flag == 0)
                                paragraph = strtok(NULL, "\n");
                        }

                        char[strlen(paragraph) - 9] prog;
                        strncpy(prog, paragraph + 9, (strlen(paragraph) - 9));
                        prog[strlen(paragraph) - 9] = 0;

                        if (!found_in(list, prog)){
                            i = 0;
                            while (list[i] != 0)
                            {
                                i++;
                            }
                            strcpy(list[i], prog);
                        }

                        free(paragraph);
                        free(read_buf);
                    }
                    closedir(d);

                    char *buf = malloc(sizeof(char*));
                    strcat(buf, "Stats_uniq:/n")
                    for (int j = 0; j < i; j++) {
                        strcat(buf, list[j]);
                        strcat(buf, "/n");
                    }

                    file_desc fifo = open(request->response_fifo_name, WRONLY);
                    int ret_val = write(fifo, buf, strlen(buf));
                    THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error writing to FIFO\n");

                    close(fifo);
                    free(buf);
                }
            }
            write(STDOUT_FILENO, "Request_Stats_Uniq Forked", 26);
        }
        default:
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
