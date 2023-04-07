#ifndef RULE_HYPERGRAPH_H
#define RULE_HYPERGRAPH_H

/**
 * 1 enables testing, 0 disables testing.
*/
#define RULE_HYPERGRAPH_TEST 0

#include "rule.h"
#include "knowledge_base.h"
#include "rule_queue.h"

/**
 * @brief Checks two RuleHyperGraphs to determine if they are equal or not. Do not use the same
 * pointer on both parameters.
 *
 * @param X The first RuleHyperGraph to compare.
 * @param Y The second RuleHyperGraph to compare.
*/
#define ck_assert_rule_hypergraph_eq(X, Y)

#define _ck_assert_rule_hypergraph_empty(X, OP)

/**
 * @brief Checks if a RuleHyperGraph is empty.
 *
 * @param X The RuleHyperGraph to check.
*/
#define ck_assert_rule_hypergraph_empty(X) _ck_assert_rule_hypergraph_empty(X, ==)

/**
 * @brief Checks if a RuleHyperGraph is not empty.
 *
 * @param X The RuleHyperGraph to check.
*/
#define ck_assert_rule_hypergraph_notempty(X) _ck_assert_rule_hypergraph_empty(X, !=)

struct KnowledgeBase;

typedef struct RuleHyperGraph RuleHyperGraph;

RuleHyperGraph *rule_hypergraph_empty_constructor();
void rule_hypergraph_destructor(RuleHyperGraph ** const rule_hypergraph);
void rule_hypergraph_copy(RuleHyperGraph ** const destination, const RuleHyperGraph * const source);
int rule_hypergraph_add_rule(RuleHyperGraph * const rule_hypergraph, Rule ** const rule);
void rule_hypergraph_remove_rule(RuleHyperGraph * const rule_hypergraph, Rule * const rule);
void rule_hypergraph_get_inactive_rules(struct KnowledgeBase * const knowledge_base,
RuleQueue ** const inactive_rules);
void rule_hypergraph_update_rules(struct KnowledgeBase * const knowledge_base,
const Scene * const observations, const Scene * const inferences, const float promotion_rate,
const float demotion_rate);

#endif
