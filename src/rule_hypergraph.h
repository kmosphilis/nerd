#include "rule.h"

#ifndef RULE_HYPERGRAPH_H
#define RULE_HYPERGRAPH_H
#include "rule.h"
#include "knowledge_base.h"
#include "rule_queue.h"

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
