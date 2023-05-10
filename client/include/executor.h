#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../../common/include/utils.h"

int executor(const char *command, file_desc fifo, file_desc client_fifo, char *client_fifo_name);

int execute_status(file_desc fifo, file_desc client_fifo, char *client_fifo_name);

int execute_stats_time(file_desc fifo, file_desc client_fifo, char *client_fifo_name, char *pids);

int execute_stats_command(file_desc fifo, file_desc client_fifo, char *client_fifo_name, char *command);

int execute_stats_uniq(file_desc fifo, file_desc client_fifo, char *client_fifo_name);

#endif
