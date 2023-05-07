#ifndef HASHTABLE_H
#define HASHTABLE_H

#define SIZE 64

#include "../include/request.h"

typedef struct node {
    int key;
    // int file_index; // value
    Request *request;
    struct node *next;
} Node;

typedef struct hashtable {
    Node **table;
} Hashtable;

Hashtable *create_hashtable();

void insert(Hashtable *ht, int key, Request *request);

Request *get_request(Hashtable *ht, int key);

void delete(Hashtable *ht, int key);

void print_ht(Hashtable *ht);

void free_ht(Hashtable *ht);

#endif
