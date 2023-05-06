#ifndef HASHTABLE_H
#define HASHTABLE_H

#define SIZE 64

typedef struct node {
    char *key;
    int file_index; // value
    struct node *next;
} Node;

typedef struct hashtable {
    Node **table;
} Hashtable;

Hashtable *create_hashtable();

int insert(Hashtable *ht, char *key);

int get_file_index(Hashtable *ht, char *key);

void print_ht(Hashtable *ht);

void free_ht(Hashtable *ht);

#endif
