#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../include/executor.h"
#include "../../common/include/utils.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        ssize_t written_bytes = write(STDERR_FILENO, "Invalid Arguments!\n Use: ./main-program execute -u \"prog arg-1 ... arg-n\"\n", 74);
        THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to stderr\n");
    }
    
    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u")) {
        int ret_val = executor(argv[3]);
        THROW_ERROR_IF_FAILED_WITH_RETURN(ret_val == -1, "Error executing command\n");
    }

    file_desc fifo = open(FIFO_NAME, O_WRONLY);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    while (1) {
        ssize_t written_bytes = write(fifo, argv[1], strlen(argv[1]));
        THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to FIFO\n");
    }

    close(fifo);
    return 0;
}
