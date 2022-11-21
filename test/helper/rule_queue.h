#include <check.h>

#include "../../src/rule_queue.h"
#include "rule.h"

#ifndef __RULE_QUEUE_HELPER_H__
#define __RULE_QUEUE_HELPER_H__

#define RULES_TO_CREATE 3

/**
 * @brief Check two RuleQueues to determine if they are equal (rule_queue1 == rule_queue2).
 * 
 * @param rule_queue1 The first RuleQueue to compare.
 * @param rule_queue2 The second RuleQueue to compare.
 */
#define ck_assert_rule_queue_eq(X, Y) do { \
    const RuleQueue * const rq1 = (X); \
    const RuleQueue * const rq2 = (Y); \
    ck_assert_int_eq(rq1->length, rq2->length); \
    unsigned int i; \
    for (i = 0; i < rq1->length; ++i) {\
        ck_assert_rule_eq(rq1->rules[i], rq2->rules[i]); \
    } \
} while (0)

#define _ck_assert_rule_queue_empty(X, OP) do { \
    const RuleQueue * const rq = (X); \
    _ck_assert_int(rq->length, OP, 0); \
    _ck_assert_ptr_null(rq->rules, OP); \
} while (0)

/**
 * @brief Check if a RuleQueue is empty.
 * 
 * @param rule_queue The RuleQueue to check.
 */
#define ck_assert_rule_queue_empty(X) _ck_assert_rule_queue_empty(X, ==)

/**
 * @brief Check if a RuleQueue is empty.
 * 
 * @param rule_queue The RuleQueue to check.
 */
#define ck_assert_rule_queue_notempty(X) _ck_assert_rule_queue_empty(X, !=)

Rule **create_rules();
void destruct_rules(Rule **rules);
void create_rule_queue(RuleQueue **rule_queue);

#endif