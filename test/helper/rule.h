#include <check.h>

#include "../../src/rule.h"
#include "literal.h"
#include "scene.h"

#ifndef __RULE_HELPER_H__
#define __RULE_HELPER_H__

/**
 * @brief Check two Rules to determine if they are equal (X == Y).
 *
 * @param X The first Rule to compare.
 * @param Y The second Rule to compare.
 */
#define ck_assert_rule_eq(X, Y) do { \
    const Rule * const r1 = (X); \
    const Rule * const r2 = (Y); \
    ck_assert_ptr_nonnull(r1); \
    ck_assert_ptr_nonnull(r2); \
    ck_assert_ptr_ne(&(r1->body), &(r2->body)); \
    ck_assert_ptr_ne(&(r1->head), &(r2->head)); \
    ck_assert_scene_eq(&(r1->body), &(r2->body)); \
    ck_assert_literal_eq(&(r1->head), &(r2->head)); \
    ck_assert_float_eq_tol(r1->weight, r2->weight, 0.000001); \
} while (0)

#define _ck_assert_rule_empty(X, OP) do { \
    const Rule * const r = (X); \
    ck_assert_ptr_nonnull(r); \
    _ck_assert_scene_empty(&(r->body), OP); \
    _ck_assert_literal_empty(&(r->head), OP); \
    _ck_assert_floating(r->weight, OP, INFINITY, float, ""); \
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
