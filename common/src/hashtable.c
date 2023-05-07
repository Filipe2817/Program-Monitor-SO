#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
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
            printf("Program: %s\n", current->request->program);
            printf("Timestamp: %s\n", current->request->timestamp);
            printf("Execution time: %ld\n", current->request->execution_time);
            printf("Response FIFO name: %s\n", current->request->response_fifo_name);
            printf("Request total size: %d\n", current->request->program_size);
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
