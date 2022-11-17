#include "../../src/literal.h"

#ifndef __LITERAL_HELPER_H__
#define __LITERAL_HELPER_H__

#define _ck_assert_literal(X, OP, Y) do { \
    const Literal * const l1 = (X); \
    const Literal * const l2 = (Y); \
    char *l1_str = literal_to_string(l1); \
    char *l2_str = literal_to_string(l2); \
    ck_assert_msg(((l1->sign OP l2->sign) || 0 OP strcmp(l1->atom, l2->atom)), \
    "Assertion '%s' failed: %s == %s, %s == %s", #X" "#OP" "#Y, #X, l1_str, #Y, l2_str); \
    free(l1_str); \
    free(l2_str); \
} while (0)

/**
 * @brief Check two Literals to determine if they are equal (literal1 == literal2).
 * 
 * @param X The first Literal to compare.
 * @param Y The second Literal to compare.
 */
#define ck_assert_literal_eq(X, Y) _ck_assert_literal(X, ==, Y)

/**
 * @brief Check two Literals to determine if they are not equal (literal1 != literal2).
 * 
 * @param X The first Literal to compare.
 * @param Y The second Literal to compare.
 */
#define ck_assert_literal_ne(X, Y) _ck_assert_literal(X, !=, Y)

#define _ck_assert_literal_empty(X, OP) do { \
    const Literal * const l = (X); \
    _ck_assert_int(l->sign, OP, 0); \
    _ck_assert_ptr_null(l->atom, OP); \
} while (0)

/**
 * @brief Check if a Literal is empty.
 * 
 * @param X The Literal to check.
 */
#define ck_assert_literal_empty(X) _ck_assert_literal_empty(X, ==)

/**
 * @brief Check if a Literal is not empty.
 * 
 * @param X The Literal to check.
 */
#define ck_assert_literal_notempty(X) _ck_assert_literal_empty(X, !=)

#endif