#ifndef NERD_UTILS_H
#define NERD_UTILS_H

#include <pcg_variants.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void _safe_free(void **ptr);

/**
 * @brief Frees the allocated pointer and sets its value to NULL.
 */
#define safe_free(ptr) _safe_free((void **)&ptr);

char *trim(const char *const string);
int train_test_split(FILE *dataset, const bool has_header,
                     const float test_ratio, pcg32_random_t *generator,
                     const char *const train_path, const char *const test_path,
                     FILE **train, FILE **test);

/* IntVector */

typedef struct IntVector {
  int *items;
  size_t size;
} IntVector;

IntVector *int_vector_constructor();
void int_vector_destructor(IntVector **const int_vector);
void int_vector_copy(IntVector **const destination,
                     const IntVector *const restrict source);
void int_vector_resize(IntVector *const int_vector,
                       const unsigned int new_size);
void int_vector_push(IntVector *const int_vector, const int item);
void int_vector_insert(IntVector *const int_vector, const unsigned int position,
                       const int item);
void int_vector_delete(IntVector *const int_vector, const unsigned int index);
int int_vector_get(const IntVector *const int_vector, const unsigned int index);
int int_vector_set(IntVector *const int_vector, const unsigned int index,
                   const int new_item);

#endif
