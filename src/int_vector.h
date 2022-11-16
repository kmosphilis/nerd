#ifndef __INT_VECTOR__H__
#define __INT_VECTOR__H__

typedef struct IntVector {
    int *items;
    unsigned int size;
} IntVector;

void int_vector_constructor(IntVector * const int_vector);
void int_vector_destructor(IntVector * const int_vector);
void int_vector_copy(IntVector * const restrict destination,
const IntVector * const restrict source);
void int_vector_resize(IntVector * const int_vector, const unsigned int new_size);
void int_vector_push(IntVector * const int_vector, const int element);
void int_vector_delete(IntVector * const int_vector, unsigned int index);
int int_vector_get(const IntVector * const int_vector, const unsigned int index);
int int_vector_set(IntVector * const int_vector, const unsigned int index, const int new_item);

#endif