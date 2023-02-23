#include <check.h>

#include "../../src/knowledge_base.h"
#include "rule_queue.h"

#ifndef KNOWLEDGE_BASE_HELPER_H
#define KNOWLEDGE_BASE_HELPER_H

/**
 * @brief Check two KnowledgeBases to determine if they are equal (X == Y).
 *
 * @param X The first KnowledgeBase to compare.
 * @param Y The second KnowledgeBase to compare.
 */
#define ck_assert_knowledge_base_eq(X, Y) do { \
    const KnowledgeBase * const _kb1 = (X); \
    const KnowledgeBase * const _kb2 = (Y); \
    ck_assert_ptr_nonnull(_kb1); \
    ck_assert_ptr_nonnull(_kb2); \
    ck_assert_float_eq_tol(_kb1->activation_threshold, _kb2->activation_threshold, 0.000001); \
    ck_assert_rule_queue_eq(_kb1->active, _kb2->active); \
    ck_assert_rule_queue_eq(_kb1->inactive, _kb2->inactive); \
} while (0)

#define _ck_assert_knowledge_base_empty(X, OP) do { \
    const KnowledgeBase * const _kb = (X); \
    ck_assert_ptr_nonnull(_kb); \
    _ck_assert_rule_queue_empty(_kb->active, OP); \
    _ck_assert_rule_queue_empty(_kb->inactive, OP); \
} while (0)

/**
 * @brief Check if a KnowledgeBase is empty.
 *
 * @param knowledge_base The KnowledgeBase to check.
 */
#define ck_assert_knowledge_base_empty(X) _ck_assert_knowledge_base_empty(X, ==)

/**
 * @brief Check if a KnowledgeBase is not empty.
 *
 * @param knowledge_base The KnowledgeBase to check.
 */
#define ck_assert_knowledge_base_notempty(X) _ck_assert_knowledge_base_empty(X, !=)

#endif
