#include "rule.h"

#ifndef RULE_HYPERGRAPH_H
#define RULE_HYPERGRAPH_H

typedef struct RuleHyperGraph RuleHyperGraph;

RuleHyperGraph *rule_hypergraph_empty_constructor();
void rule_hypergraph_destructor(RuleHyperGraph ** const rule_hypergraph);
int rule_hypergraph_add_rule(RuleHyperGraph * const rule_hypergraph, Rule ** const rule);
void rule_hypergraph_remove_rule(RuleHyperGraph * const rule_hypergraph, Rule * const rule);

#endif
