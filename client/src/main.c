#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../include/executor.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        ssize_t written_bytes = write(STDERR_FILENO, "Invalid Arguments!\n Use: ./main-program execute -u \"prog arg-1 ... arg-n\"\n", 74);
        if (written_bytes == -1) {
            perror("Error writing to stderr");
        }
        return 1;
    }
    
    if (!strcmp(argv[1], "execute") && !strcmp(argv[2], "-u")) {
        int ret_val = executor(argv[3]);
        if (ret_val == -1) {
            perror("Error executing command");
            return 1;
        }
    }

    return 0;
}
