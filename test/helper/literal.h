#include <check.h>

#include "../../src/literal.h"

#ifndef __LITERAL_HELPER_H__
#define __LITERAL_HELPER_H__

#define _ck_assert_literal(X, OP, Y) do { \
    const Literal * const l1 = (X); \
    const Literal * const l2 = (Y); \
    if (!l1) { \
        ck_assert_ptr_null(l1); \
    } else if (!l2) { \
        ck_assert_ptr_null(l2); \
    } else { \
        char l1_sign = l1->sign ? ' ' : '-'; \
        char l2_sign = l2->sign ? ' ' : '-'; \
        ck_assert_msg(((l1->sign OP l2->sign) || (!l1->atom) || (!l2->atom) || \
        (0 OP strcmp(l1->atom, l2->atom))), "Assertion 'literal1%sliteral2' failed: literal1 = " \
        "%c%s, literal2 = %c%s", " "#OP" ", l1_sign, l1->atom, l2_sign, l2->atom); \
    } \
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
    if (!l) { \
        ck_assert_ptr_null(l); \
    } else { \
        _ck_assert_ptr_null(l->atom, OP); \
    } \
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
