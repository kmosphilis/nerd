#include <check.h>

#include "../../src/rule_queue.h"
#include "rule.h"

#ifndef __RULE_QUEUE_HELPER_H__
#define __RULE_QUEUE_HELPER_H__

#define RULES_TO_CREATE 3


/**
 * @brief Check two RuleQueues to determine if they are equal (X == Y).
 *
 * @param X The first RuleQueue to compare.
 * @param Y The second RuleQueue to compare.
 */
#define ck_assert_rule_queue_eq(X, Y) do { \
    const RuleQueue * const _rq1 = (X); \
    const RuleQueue * const _rq2 = (Y); \
    ck_assert_ptr_nonnull(_rq1); \
    ck_assert_ptr_nonnull(_rq2); \
    ck_assert_int_eq(_rq1->length, _rq2->length); \
    if (_rq1->length == 0) { \
        ck_assert_ptr_null(_rq1->rules); \
        ck_assert_ptr_null(_rq2->rules); \
    } else { \
        ck_assert_ptr_ne(_rq1->rules, _rq2->rules); \
        unsigned int i; \
        for (i = 0; i < _rq1->length; ++i) { \
            ck_assert_rule_eq(&(_rq1->rules[i]), &(_rq2->rules[i])); \
        } \
    } \
} while (0)

#define _ck_assert_rule_queue_empty(X, OP) do { \
    const RuleQueue * const _rq = (X); \
    ck_assert_ptr_nonnull(_rq); \
    _ck_assert_int(_rq->length, OP, 0); \
    _ck_assert_ptr_null(_rq->rules, OP); \
} while (0)

/**
 * @brief Check if a RuleQueue is empty.
 *
 * @param rule_queue The RuleQueue to check.
 */
#define ck_assert_rule_queue_empty(X) _ck_assert_rule_queue_empty(X, ==)

/**
 * @brief Check if a RuleQueue is not empty.
 *
 * @param X The RuleQueue to check.
 */
#define ck_assert_rule_queue_notempty(X) _ck_assert_rule_queue_empty(X, !=)

void create_rule_queue(RuleQueue * const rule_queue);
Rule *create_rules();
void destruct_rules(Rule * const rules);

#endif
