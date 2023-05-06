#ifndef EXECUTOR_H
#define EXECUTOR_H

int executor(const char *command, file_desc fifo, file_desc client_fifo, char *client_fifo_name);

#endif
