#include <check.h>

#include "rule_queue.h"

#ifndef __KNOWLEDGE_BASE_HELPER_H__
#define __KNOWLEDGE_BASE_HELPER_H__

/**
 * @brief Check two KnowledgeBases to determine if they are equal
 * (knowledge_base1 == knowledge_base2).
 *
 * @param knowledge_base1 The first KnowledgeBase to compare.
 * @param knowledge_base2 The second KnowledgeBase to compare.
 */
#define ck_assert_knowledge_base_eq(X, Y) do { \
    const KnowledgeBase * const kb1 = (X); \
    const KnowledgeBase * const kb2 = (Y); \
    ck_assert_float_eq_tol(kb1->activation_threshold, kb2->activation_threshold, 0.00001); \
    ck_assert_rule_queue_eq(kb1->active, kb2->active); \
    ck_assert_rule_queue_eq(kb1->inactive, kb2->inactive); \
} while (0)

#define _ck_assert_knowledge_base_empty(X, OP) do { \
    const KnowledgeBase * const kb = (X) ; \
    _ck_assert_int(kb->active->length, OP, 0); \
    _ck_assert_ptr_null(kb->active->rules, OP); \
    _ck_assert_int(kb->inactive->length, OP, 0); \
    _ck_assert_ptr_null(kb->inactive->rules, OP); \
} while (0)

/**
 * @brief Check if a KnowledgeBase is not empty.
 *
 * @param X The KnowledgeBase to check.
 */
#define ck_assert_knowledge_base_notempty(X) _ck_assert_knowledge_base_empty(X, !=)

/**
 * @brief Check if a KnowledgeBase is empty.
 *
 * @param knowledge_base The KnowledgeBase to check.
 */
#define ck_assert_knowledge_base_empty(X) _ck_assert_knowledge_base_empty(X, ==)

#endif