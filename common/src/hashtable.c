#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../include/hashtable.h"

uint32_t hash(char *str) { // variant of djb2
    uint32_t result = 5381;
    unsigned char *p = (unsigned char *)str;

    while (*p)
        result = ((result << 5) ^ result) ^ (*p++);

    return result % SIZE;
}

Node *create_node(char *key, int **index_ptr) {
    static int index = 0;
    Node *new = malloc(sizeof(struct node));
    if (new == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for node!\n");
        exit(EXIT_FAILURE);
    }
    new->key = key;
    new->file_index = index++;
    new->next = NULL;
    *index_ptr = &new->file_index;
    return new;
}

Hashtable *create_hashtable() {
    Hashtable *new = malloc(sizeof(struct hashtable));

    if (new == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for hashtable!\n");
        exit(EXIT_FAILURE);
    }

    new->table = malloc(sizeof(Node *) * SIZE);

    if (new->table == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for hashtable array!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < SIZE; i++)
        new->table[i] = NULL;

    return new;
}

int insert(Hashtable *ht, char *key) {
    uint32_t index = hash(key);
    int *file_index = NULL;

    Node *new = create_node(key, &file_index);
    new->next = ht->table[index];
    ht->table[index] = new;

    return *file_index;
}

int get_file_index(Hashtable *ht, char *key) {
    uint32_t index = hash(key);
    Node *current = ht->table[index];

    while (current != NULL) {
        if (!strcmp(current->key, key))
            return current->file_index;
        current = current->next;
    }

    return -1;
}

void delete(Hashtable *ht, char *key) {
    uint32_t index = hash(key);
    Node **current = &(ht->table[index]);

    while (*current != NULL) {
        if (!strcmp((*current)->key, key)) {
            Node *temp = *current;
            *current = (*current)->next;
            free(temp);
            return;
        }
        current = &((*current)->next);
    }
}

void print_ht(Hashtable *ht) {
    for (int i = 0; i < SIZE; i++) {
        printf("[%d] | ", i);
        Node *current = ht->table[i];
        while (current != NULL) {
            printf("(Key: %s, Index: %d) -> ", current->key, current->file_index);
            current = current->next;
        }
        printf("NULL\n");
    }
}

void free_ht(Hashtable *ht) {
    for (int i = 0; i < SIZE; i++) {
        Node *current = ht->table[i];
        while (current != NULL) {
            Node *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}
