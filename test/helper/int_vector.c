#include <check.h>

#include "int_vector.h"

/**
 * @brief Check if an IntVector is not empty.
 * 
 * @param int_vector The IntVector to check.
 * 
 */
void ck_assert_int_vector_notempty(const IntVector * restrict int_vector) {
    ck_assert_int_ne(int_vector->size, 0);
    ck_assert_ptr_nonnull(int_vector->items);
}

/**
 * @brief Check if two IntVectors are equal (int_vector1 == int_vector2).
 * 
 * @param int_vector1 The first IntVector to compare.
 * @param int_vector2 The second IntVector to compare.
 */
void ck_assert_int_vector_eq(const IntVector * restrict int_vector1,
const IntVector * restrict int_vector2) {
    ck_assert_int_eq(int_vector1->size, int_vector2->size);
    ck_assert_int_vector_notempty(int_vector1);
    ck_assert_int_vector_notempty(int_vector2);
    
    unsigned int i;
    for (i = 0; i < int_vector1->size; ++i) {
        ck_assert_int_eq(int_vector1->items[i], int_vector2->items[i]);
    }
}

/**
 * @brief Check if an IntVector is empty.
 * 
 * @param int_vector The IntVector to check.
 */
void ck_assert_int_vector_empty(const IntVector * restrict int_vector) {
    ck_assert_int_eq(int_vector->size, 0);
    ck_assert_ptr_null(int_vector->items);
}