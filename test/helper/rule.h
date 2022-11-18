#include <check.h>

#include "literal.h"
#include "scene.h"

#ifndef __RULE_HELPER_H__
#define __RULE_HELPER_H__

/**
 * @brief Check two Rules to determine if they are equal (rule1 == rule2).
 * 
 * @param X The first Rule to compare.
 * @param Y The second Rule to compare.
 */
#define ck_assert_rule_eq(X, Y) do { \
    const Rule * const r1 = (X); \
    const Rule * const r2 = (Y); \
    ck_assert_scene_eq(r1->body, r2->body); \
    ck_assert_literal_eq(r1->head, r2->head); \
    ck_assert_float_eq_tol(r1->weight, r2->weight, 0.000001); \
} while (0)

#endif