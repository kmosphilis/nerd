#include <check.h>

#include "../../src/literal.h"

#ifndef LITERAL_TEST_HELPER_H
#define LITERAL_TEST_HELPER_H

#define _ck_assert_literal(X, OP, Y)                                           \
  do {                                                                         \
    const Literal *const _l1 = (X);                                            \
    const Literal *const _l2 = (Y);                                            \
    ck_assert_ptr_nonnull(_l1);                                                \
    ck_assert_ptr_nonnull(_l2);                                                \
    char l1_sign = _l1->sign ? ' ' : '-';                                      \
    char l2_sign = _l2->sign ? ' ' : '-';                                      \
    if (_l1->atom) {                                                           \
      if (_l2->atom) {                                                         \
        ck_assert_msg(                                                         \
            ((_l1->sign OP _l2->sign) || (0 OP strcmp(_l1->atom, _l2->atom))), \
            "Assertion 'literal1%sliteral2' failed: literal1 = %c%s, "         \
            "literal2 = %c%s",                                                 \
            " " #OP " ", l1_sign, _l1->atom, l2_sign, _l2->atom);              \
      } else {                                                                 \
        ck_assert_literal_empty(_l2);                                          \
      }                                                                        \
    } else {                                                                   \
      if (!_l2->atom) {                                                        \
        ck_assert_literal_empty(_l2);                                          \
      }                                                                        \
      ck_assert_literal_empty(_l1);                                            \
    }                                                                          \
  } while (0)

/**
 * @brief Check two Literals to determine if they are equal (X == Y).
 *
 * @param X The first Literal to compare.
 * @param Y The second Literal to compare.
 */
#define ck_assert_literal_eq(X, Y) _ck_assert_literal(X, ==, Y)

/**
 * @brief Check two Literals to determine if they are not equal (X != Y).
 *
 * @param X The first Literal to compare.
 * @param Y The second Literal to compare.
 */
#define ck_assert_literal_ne(X, Y) _ck_assert_literal(X, !=, Y)

#define _ck_assert_literal_empty(X, OP)                                        \
  do {                                                                         \
    const Literal *const _l = (X);                                             \
    ck_assert_ptr_nonnull(_l);                                                 \
    _ck_assert_ptr_null(_l->atom, OP);                                         \
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
