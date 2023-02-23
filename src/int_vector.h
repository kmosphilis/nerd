#include <stdlib.h>

#ifndef INT_VECTOR__H
#define INT_VECTOR__H

typedef struct IntVector {
    int *items;
    size_t size;
} IntVector;

IntVector *int_vector_constructor();
void int_vector_destructor(IntVector ** const int_vector);
void int_vector_copy(IntVector ** const destination, const IntVector * const restrict source);
void int_vector_resize(IntVector * const int_vector, const unsigned int new_size);
void int_vector_push(IntVector * const int_vector, const int item);
void int_vector_insert(IntVector * const int_vector, const unsigned int position, const int item);
void int_vector_delete(IntVector * const int_vector, const unsigned int index);
int int_vector_get(const IntVector * const int_vector, const unsigned int index);
int int_vector_set(IntVector * const int_vector, const unsigned int index, const int new_item);

#endif
