#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

Node *create_node(int key, Request request)
{
    static int index = 0;
    Node *new = malloc(sizeof(struct node));

    THROW_ERROR_IF_FAILED_VOID(new, "Error: Unable to allocate memory for node!\n");

    new->key = key;
    new->request = request;
    new->next = NULL;
    //*index_ptr = &new->file_index;
    return new;
}

Hashtable *create_hashtable()
{
    Hashtable *new = malloc(sizeof(struct hashtable));

    THROW_ERROR_IF_FAILED_VOID(new, "Error: Unable to allocate memory for hashtable!\n");

    new->table = malloc(sizeof(Node *) * SIZE);

    THROW_ERROR_IF_FAILED_VOID(new->table, "Error: Unable to allocate memory for hashtable array!\n");

    for (int i = 0; i < SIZE; i++)
        new->table[i] = NULL;

    return new;
}

void insert(Hashtable *ht, int key, Request request)
{
    uint32_t index = hash(key);
    // int *file_index = NULL;

    Node *new = create_node(key, request);
    new->next = ht->table[index];
    ht->table[index] = new;

    // return *file_index;
}

Request get_request(Hashtable *ht, int key)
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
            free(temp);
            return;
        }
        current = &((*current)->next);
    }
}

// void print_ht(Hashtable *ht)
// {
//     for (int i = 0; i < SIZE; i++)
//     {
//         printf("[%d] | ", i);
//         Node *current = ht->table[i];
//         while (current != NULL)
//         {
//             printf("(Key: %s, Index: %d) -> ", current->key, current->file_index);
//             current = current->next;
//         }
//         printf("NULL\n");
//     }
// }

void free_ht(Hashtable *ht)
{
    for (int i = 0; i < SIZE; i++)
    {
        Node *current = ht->table[i];
        while (current != NULL)
        {
            Node *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}
