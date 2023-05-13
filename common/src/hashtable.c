#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "../include/hashtable.h"
#include "../include/utils.h"

// uint32_t hash(char *str) { // variant of djb2
//     uint32_t result = 5381;
//     unsigned char *p = (unsigned char *)str;
//
//     while (*p)
//         result = ((result << 5) ^ result) ^ (*p++);
//
//     return result % SIZE;
// }

unsigned int hash(unsigned int x)
{
    return x % SIZE;
}

Node *create_node(int key, Request *request)
{
    // static int index = 0;
    Node *new = malloc(sizeof(struct node));

    if (new == NULL)
    {
        perror("Error: Unable to allocate memory for node!\n");
        exit(EXIT_FAILURE);
    }

    new->key = key;
    new->request = request;
    new->next = NULL;
    //*index_ptr = &new->file_index;
    return new;
}

Hashtable *create_hashtable()
{
    Hashtable *new = malloc(sizeof(struct hashtable));

    if (new == NULL)
    {
        perror("Error: Unable to allocate memory for node!\n");
        exit(EXIT_FAILURE);
    }

    new->table = malloc(sizeof(Node *) * SIZE);

    if (new->table == NULL)
    {
        perror("Error: Unable to allocate memory for hashtable array!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < SIZE; i++)
        new->table[i] = NULL;

    return new;
}

void insert(Hashtable *ht, int key, Request *request)
{
    uint32_t index = hash(key);
    // int *file_index = NULL;

    Node *new = create_node(key, request);
    new->next = ht->table[index];
    ht->table[index] = new;

    // return *file_index;
}

Request *get_request(Hashtable *ht, int key)
{
    uint32_t index = hash(key);
    Node *current = ht->table[index];

    while (current != NULL)
    {
        if (current->key == key)
            return current->request;
        current = current->next;
    }

    perror("Nothing found.\n");
    exit(0); // suspeito
}

void delete(Hashtable *ht, int key)
{
    uint32_t index = hash(key);
    Node **current = &(ht->table[index]);

    while (*current != NULL)
    {
        if ((*current)->key == key)
        {
            Node *temp = *current;
            *current = (*current)->next;
            delete_request(temp->request);
            free(temp);
            return;
        }
        current = &((*current)->next);
    }
}

char *get_ongoing_programs(Hashtable *ht)
{
    char *status = malloc(100 * sizeof(char));
    status[0] = '\0';
    int size = 100;
    int len = 0;

    for (int i = 0; i < SIZE; i++)
    {
        Node *current = ht->table[i];
        while (current != NULL)
        {
            int pid_digits = floor(log10(current->request->pid)) + 1;

            char timestamp[32];
            get_timestamp(timestamp, sizeof(timestamp));

            int elapsed_time = get_diff_milliseconds(current->request->timestamp, timestamp);

            int elapsed_time_digits = floor(log10(elapsed_time)) + 1;

            int line_len = pid_digits + elapsed_time_digits + current->request->payload_size + 6; // 6 dos 3 espaços + \n + "ms"

            char *status_line = malloc(line_len * sizeof(char));

            snprintf(status_line, line_len, "%d %s %d ms\n", current->request->pid, current->request->payload, elapsed_time);

            if ((len += line_len) >= size)
            {
                size *= 2;
                status = realloc(status, size * sizeof(char));
                if (status == NULL)
                {
                    perror("Error during realloc\n");
                    return status;
                }

                strcat(status, status_line);
            }
            else
            {
                len += line_len;
                strcat(status, status_line);
            }

            free(status_line);

            current = current->next;
        }
    }

    if (len == 0)
    {
        strcat(status, "No ongoing programs.\n");
    }

    return status;
}

void print_ht(Hashtable *ht)
{
    for (int i = 0; i < SIZE; i++)
    {
        Node *current = ht->table[i];
        while (current != NULL)
        {
            printf("----------> [%d] <---------- \n", i);
            printf("(Key: %d)\n", current->key);
            printf("Type: %d\n", current->request->type);
            printf("PID: %d\n", current->request->pid);
            printf("Program: %s\n", current->request->payload);
            printf("Timestamp: %s\n", current->request->timestamp);
            printf("Execution time: %ld\n", current->request->execution_time);
            printf("Response FIFO name: %s\n", current->request->response_fifo_name);
            printf("Request total size: %d\n", current->request->request_total_size);
            printf("REQUEST PRINTED\n");
            current = current->next;
        }
    }
    printf("Hashtable printed.\n\n");
}

void free_ht(Hashtable *ht)
{
    for (int i = 0; i < SIZE; i++)
    {
        Node *current = ht->table[i];
        while (current != NULL)
        {
            Node *temp = current;
            current = current->next;
            delete_request(temp->request);
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}
