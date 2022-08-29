#include <check.h>

#include "knowledge_base.h"
#include "rule_queue.h"

/**
 * @brief Check two KnowledgeBases to determine if they are equal 
 * (knowledge_base1 == knowledge_base2).
 * 
 * @param knowledge_base1 The first KnowledgeBase to compare.
 * @param knowledge_base2 The second KnowledgeBase to compare.
 */
void ck_assert_knowledge_base_eq(const KnowledgeBase * const knowledge_base1,
const KnowledgeBase * const knowledge_base2) {
    ck_assert_float_eq_tol(knowledge_base1->activation_threshold,
    knowledge_base2->activation_threshold, 0.00001);
    ck_assert_rule_queue_eq(&(knowledge_base1->active), &(knowledge_base2->active));
    ck_assert_rule_queue_eq(&(knowledge_base1->inactive), &(knowledge_base2->inactive));
}

/**
 * @brief Check if a KnowledgeBase is not empty.
 * 
 * @param knowledge_base The KnowledgeBase to check.
 */
void ck_assert_knowledge_base_notempty(const KnowledgeBase * const knowledge_base) {
    ck_assert_float_ne(knowledge_base->activation_threshold, INFINITY);
    ck_assert_rule_queue_notempty(&(knowledge_base->active));
    ck_assert_rule_queue_notempty(&(knowledge_base->inactive));
}

/**
 * @brief Check if a KnowledgeBase is empty.
 * 
 * @param knowledge_base The KnowledgeBase to check.
 */
void ck_assert_knowledge_base_empty(const KnowledgeBase * const knowledge_base) {
    ck_assert_float_eq(knowledge_base->activation_threshold, INFINITY);
    ck_assert_rule_queue_empty(&(knowledge_base->active));
    ck_assert_rule_queue_empty(&(knowledge_base->inactive));
}