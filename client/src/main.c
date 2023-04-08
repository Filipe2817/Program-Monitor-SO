#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../include/parser.h"

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "execute") || strcmp(argv[2], "-u")) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        write(STDERR_FILENO, "Invalid Arguments!\n Use: ./main-program execute -u \"prog arg-1 ... arg-n\"\n", 74);
        #pragma GCC diagnostic pop
        return 1;
    } else {
        parse_cmd(argv[3]);
    }

    int *mem = malloc(100 * sizeof(int)); // test sanitizer

    return 0;
}
