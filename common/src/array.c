#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/array.h"
#include "../include/utils.h"

Array *create_array() {
    Array *a = malloc(sizeof(struct array));
    a->array = malloc(INITIAL_SIZE * sizeof(int));
    a->used = 0;
    a->size = INITIAL_SIZE;
    return a;
}

void insert_array(Array *a, int element) {
    if (a->used == a->size) {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++] = element;
}

void sort_array(Array *a) {
    qsort(a->array, a->used, sizeof(int), compare_ints);
}

void delete_array(Array *a) {
    free(a->array);
    free(a);
}
