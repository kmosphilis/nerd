#include <check.h>

#ifndef __INT_VECTOR_HELPER_H__
#define __INT_VECTOR_HELPER_H__

/**
 * @brief Check if two IntVectors are equal (int_vector1 == int_vector2).
 * 
 * @param X The first IntVector to compare.
 * @param Y The second IntVector to compare.
 */
#define ck_assert_int_vector_eq(X, Y) do { \
    const IntVector * const v1 = (X); \
    const IntVector * const v2 = (Y); \
    unsigned int i; \
    ck_assert_int_eq(v1->size, v2->size); \
    for (i = 0; i < v1->size; ++i) { \
        ck_assert_int_eq(v1->items[i], v2->items[i]); \
    } \
} while (0)

#define _ck_assert_int_vector_empty(X, OP) do { \
    const IntVector * const v1 = (X); \
    _ck_assert_int(v1->size, OP, 0); \
    _ck_assert_ptr_null(v1->items, OP); \
} while (0)

/**
 * @brief Check if an IntVector is empty.
 * 
 * @param X The IntVector to check.
 */
#define ck_assert_int_vector_empty(X) _ck_assert_int_vector_empty(X, ==)

/**
 * @brief Check if an IntVector is empty.
 * 
 * @param X The IntVector to check.
 */
#define ck_assert_int_vector_notempty(X) _ck_assert_int_vector_empty(X, !=)

#endif