#include "rule.h"

#ifndef RULE_HYPERGRAPH_H
#define RULE_HYPERGRAPH_H

typedef struct RuleHyperGraph RuleHyperGraph;

void rule_hypergraph_empty_constructor(RuleHyperGraph * const rule_hypergraph);
void rule_hypergraph_destructor(RuleHyperGraph * const rule_hypergraph);
void rule_hypergraph_add_rule(RuleHyperGraph * const rule_hypergraph, const Rule * const rule);

#endif
