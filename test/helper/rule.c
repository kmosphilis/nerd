#include <check.h>

#include "rule.h"
#include "literal.h"

/**
 * @brief Check two Rules to determine if they are equal (rule1 == rule2).
 * 
 * @param rule1 The first Rule to compare.
 * @param rule2 The second Rule to compare.
 */
void ck_assert_rule_eq(const Rule * const rule1, const Rule * const rule2) {
    ck_assert_int_eq(rule1->body.size, rule2->body.size);

    unsigned int i;
    for (i = 0; i < rule1->body.size; ++i) {
        ck_assert_literal_eq(&(rule1->body.observations[i]), &(rule2->body.observations[i]));
    }

    ck_assert_literal_eq(&rule1->head, &rule2->head);
    ck_assert_float_eq_tol(rule1->weight, rule2->weight, 0.000001);
}

/**
 * @brief Check if a Rule is not empty.
 * 
 * @param rule The Rule to check.
 */
void ck_assert_rule_notempty(const Rule * const rule) {
    ck_assert_int_ne(rule->body.size, 0);
    ck_assert_ptr_nonnull(rule->body.observations);
    ck_assert_float_ne(rule->weight, INFINITY);
    ck_assert_literal_notempty(&(rule->head));
}

/**
 * @brief Check if a Rule is empty.
 * 
 * @param rule The Rule to check.
 */
void ck_assert_rule_empty(const Rule * const rule) {
    ck_assert_int_eq(rule->body.size, 0);
    ck_assert_ptr_null(rule->body.observations);
    ck_assert_float_eq(rule->weight, INFINITY);
    ck_assert_literal_empty(&(rule->head));
}