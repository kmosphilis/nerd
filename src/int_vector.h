#ifndef __INT_VECTOR__H__
#define __INT_VECTOR__H__

typedef struct IntVector {
    int *items;
    unsigned int size;
} IntVector;

void int_vector_constructor(IntVector * restrict int_vector);
void int_vector_destructor(IntVector * restrict int_vector);
void int_vector_copy(IntVector * restrict destination, const IntVector * restrict source);
void int_vector_resize(IntVector * restrict int_vector, const unsigned int new_size);
void int_vector_push(IntVector * restrict int_vector, const int element);
void int_vector_delete(IntVector * restrict int_vector, unsigned int index);
int int_vector_get(const IntVector * restrict int_vector, const unsigned int index);
int int_vector_set(IntVector * restrict int_vector, const unsigned int index, const int new_item);

#endif