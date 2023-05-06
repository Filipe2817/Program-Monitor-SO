#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../../common/include/utils.h"

int executor(const char *command, file_desc fifo, file_desc client_fifo, char *client_fifo_name);

#endif
