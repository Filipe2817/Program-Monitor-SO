#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>
#include "../include/utils.h"
#include "../include/array.h"

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

int is_in_array(char **array, char *element, int size) {
    for (int i = 0; i < size; i++) {
        if (!strcmp(array[i], element))
            return 1;
    }
    return 0;
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

char *strnconcat(char *dest, char *src, int n) {
    while (*dest)
        dest++;

    while ((*dest++ = *src++) && n-- > 0);

    return --dest;
}

int str_to_int(const char *str) {
    char *endptr;
    errno = 0;

    long long_var = strtol(str, &endptr, 10);
    //   out of range   , extra junk at end,  no conversion at all
    if (errno == ERANGE || *endptr != '\0' || str == endptr) {
        perror("Something went wrong converting string to integer!\n");
        exit(EXIT_FAILURE);
    }

// Needed when `int` and `long` have different ranges
#if LONG_MIN < INT_MIN || LONG_MAX > INT_MAX
    if (long_var < INT_MIN || long_var > INT_MAX) {
        errno = ERANGE;
        perror("String value is out of range for type integer\n");
        exit(EXIT_FAILURE);
    }
#endif

    return (int)long_var;
}

int readln(file_desc fd, char *line, int size) {
    THROW_ERROR_IF_FAILED_WITH_RETURN(size <= 0, "Invalid buffer size");

    ssize_t read_bytes = read(fd, line, size);
    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes < 0, "Read failed");

    THROW_ERROR_IF_FAILED_WITH_RETURN(read_bytes == (ssize_t)size, "Buffer overflow");

    int line_len = strcspn(line, "\n") + 1;
    line[line_len] = 0;
    lseek(fd, line_len - read_bytes, SEEK_CUR);

    return line_len;
}

int compare_ints(const void *a, const void *b) {
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;

    return (arg1 < arg2) ? -1 : (arg1 > arg2);
    // return (arg1 > arg2) - (arg1 < arg2); // possible shortcut
    // return arg1 - arg2; // erroneous shortcut (fails if INT_MIN is present)
}

Array *get_file_pids(char *dir_path) {
    Array *files = create_array();
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files and directories ("." and "..")
        if (entry->d_name[0] == '.') {
            continue;
        }

        char *dot_pos = strrchr(entry->d_name, '.');
        int len = dot_pos - entry->d_name;

        char *pid = malloc(sizeof(char) * (len + 1));
        if (pid == NULL) {
            perror("Error allocating memory");
            exit(EXIT_FAILURE);
        }
        strncpy(pid, entry->d_name, len);
        pid[len] = '\0';

        insert_array(files, str_to_int(pid));
        free(pid);
    }

    closedir(dir);
    return files;
}

void convert_string_array_to_int_array(char **string_array, int *int_array, int size) {
    for (int i = 0; i < size; i++) {
        int_array[i] = str_to_int(string_array[i]);
    }
}
