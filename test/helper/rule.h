#include <check.h>

#include "../../src/rule.h"
#include "literal.h"
#include "scene.h"

#ifndef RULE_TEST_HELPER_H
#define RULE_TEST_HELPER_H

/**
 * @brief Check two Rules to determine if they are equal (X == Y).
 *
 * @param X The first Rule to compare.
 * @param Y The second Rule to compare.
 */
#define ck_assert_rule_eq(X, Y) do { \
    const Rule * const _r1 = (X); \
    const Rule * const _r2 = (Y); \
    ck_assert_ptr_nonnull(_r1); \
    ck_assert_ptr_nonnull(_r2); \
    ck_assert_scene_eq(_r1->body, _r2->body); \
    ck_assert_literal_eq(_r1->head, _r2->head); \
    ck_assert_float_eq_tol(_r1->weight, _r2->weight, 0.000001); \
} while (0)

#define _ck_assert_rule_empty(X, OP) do { \
    const Rule * const _r = (X); \
    ck_assert_ptr_nonnull(_r); \
    _ck_assert_scene_empty(_r->body, OP); \
    _ck_assert_literal_empty(_r->head, OP); \
    _ck_assert_floating(_r->weight, OP, INFINITY, float, ""); \
} while (0)

/**
 * @brief Check if a Rule is empty.
 *
 * @param X The Rule to check.
 */
#define ck_assert_rule_empty(X) _ck_assert_rule_empty(X, ==)

/**
 * @brief Check if a Rule is not empty.
 *
 * @param X The Rule to check.
 */
#define ck_assert_rule_notempty(X) _ck_assert_rule_empty(X, !=)

#endif
