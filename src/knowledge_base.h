#include "rule_queue.h"

#ifndef __KNOWLEDGE_BASE_H__
#define __KNOWLEDGE_BASE_H__

typedef struct KnowledgeBase {
    RuleQueue active, inactive;
    float activation_threshold;
} KnowledgeBase;

void knowledge_base_construct(KnowledgeBase * const knolwedge_base,
const float activation_threshold);

void knowledge_base_destruct(KnowledgeBase * const knowledge_base);
void knowledge_base_copy(KnowledgeBase * const destination, const KnowledgeBase * const source);
void knowledge_base_add_rule(KnowledgeBase * const knowledge_base, const Rule * const rule);
void knowledge_base_applicable_rules(const KnowledgeBase * const knowledge_base,
Literal ** const literals, const unsigned int literals_size, RuleQueue * const applicaple_rules);
void knowledge_base_promote_rules(KnowledgeBase * const knowledge_base,
const RuleQueue * const rules_to_promote, const float promotion_rate);
void knowledge_base_demote_rules(KnowledgeBase * const knowledge_base,
const RuleQueue * const rules_to_demote, const float demotion_rate);
char *knowledge_base_to_string(const KnowledgeBase * const knowledge_base);

#endif