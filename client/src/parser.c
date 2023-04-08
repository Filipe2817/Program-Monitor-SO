#include <string.h>
#include <unistd.h>
#include "../include/parser.h"

void parse_cmd(const char *line) {
    char *line_copy = NULL, *save_ptr = NULL, *token = NULL;

    line_copy = save_ptr = strdup(line);
    
    if (save_ptr == NULL) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        write(STDERR_FILENO, "Error: could not allocate memory for line_copy\n", 47);
        #pragma GCC diagnostic pop
        return;
    }

    while ((token = strsep(&line_copy, " ")) != NULL) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wunused-result"
        write(STDOUT_FILENO, "Token: ", 7);
        write(STDOUT_FILENO, token, strlen(token));
        write(STDOUT_FILENO, "\n", 1);
        #pragma GCC diagnostic pop
    }
}
