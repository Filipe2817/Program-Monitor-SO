#ifndef ARRAY_H
#define ARRAY_H

#define INITIAL_SIZE 8

typedef struct array {
    int *array;
    int used;
    int size;
} Array;

Array *create_array();

void insert_array(Array *a, int element);

void sort_array(Array *a);

void delete_array(Array *a);

#endif
