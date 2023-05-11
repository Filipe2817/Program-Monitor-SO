#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "../include/utils.h"

int create_new_fifo(const char *fifo_name) {
    struct stat stats;

    if (stat(fifo_name, &stats) < 0) { // Stat failed
        // If (errno == ENOENT), the file or directory does not exist
        // If (errno != ENOENT), an error occurred that is unrelated to the file or directory not existing (Stat failed)
        THROW_ERROR_IF_FAILED_WITH_RETURN(errno != ENOENT, "Stat failed\n");
    } else {
        // The fifo already exists so we need to delete it
        THROW_ERROR_IF_FAILED_WITH_RETURN(unlink(fifo_name) < 0, "Unlink failed\n");
    }

    // Create new fifo
    THROW_ERROR_IF_FAILED_WITH_RETURN(mkfifo(fifo_name, 0666) < 0, "Fifo creation failed\n");

    return 0;
}

int parse_command(const char *command, char **args, char *sep) {
    char *cmd_copy = NULL, *token = NULL;
    int argc = 0;

    cmd_copy = strdup(command);
    THROW_ERROR_IF_FAILED_WITH_RETURN(cmd_copy == NULL, "Error allocating memory\n");

    while ((token = strsep(&cmd_copy, sep)) != NULL) { // free(args[0]) -> args[0] has the same address as cmd_copy
        if (*token == '\0')
            continue; // Skip empty tokens

        if (*token == ' ') { // Space == problem to free the commands because the next parse is going to skip the space like a
            token++;         // token and the address won't be the same as the address of line_copy
        }

        args[argc++] = token;
    }

    return argc;
}

bool found_in(char **list, char *id) {

    int i = 0;
    int flag = 0;
    while (list[i] != 0 && flag == 0) {

        if (strcmp(list[i], id)) {
            flag = 1;
        } else {
            i++;
        }
    }
    if (flag == 1) {
        return true;
    }
    return false;
}

void get_timestamp(char *buffer, int size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t current_time = tv.tv_sec;
    struct tm *local_time = localtime(&current_time);
    snprintf(buffer, size, "%02d-%02d-%04d %02d:%02d:%02d.%03ld",
             local_time->tm_mday, local_time->tm_mon + 1, local_time->tm_year + 1900,
             local_time->tm_hour, local_time->tm_min, local_time->tm_sec, tv.tv_usec / 1000);
}

int get_diff_milliseconds(char *earlier_ts, char *later_ts) {
    // printf("%s\n%s\n", earlier_ts, later_ts);

    int day1, month1, year1, hour1, minute1, second1, millisecond1;
    int day2, month2, year2, hour2, minute2, second2, millisecond2;

    sscanf(earlier_ts, "%d-%d-%d %d:%d:%d.%d", &day1, &month1, &year1, &hour1, &minute1, &second1, &millisecond1);
    sscanf(later_ts, "%d-%d-%d %d:%d:%d.%d", &day2, &month2, &year2, &hour2, &minute2, &second2, &millisecond2);

    // printf("%d %d %d %d %d %d %d\n", day1, month1, year1, hour1, minute1, second1, millisecond1);
    // printf("%d %d %d %d %d %d %d\n", day2, month2, year2, hour2, minute2, second2, millisecond2);

    int diff_ms = (day2 - day1) * 24 * 60 * 60 * 1000 +
                  (month2 - month1) * 30 * 24 * 60 * 60 * 1000 +
                  (year2 - year1) * 365 * 24 * 60 * 60 * 1000 +
                  (hour2 - hour1) * 60 * 60 * 1000 +
                  (minute2 - minute1) * 60 * 1000 +
                  (second2 - second1) * 1000 +
                  (millisecond2 - millisecond1);

    return diff_ms;
}

char *str_concat(char *dest, char *src) {
    while (*dest) dest++;
    while (*dest++ = *src++);
    return --dest;
}
