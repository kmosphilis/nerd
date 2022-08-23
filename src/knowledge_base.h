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
//TODO Implement function
void knowledge_base_fire_rules(const KnowledgeBase * const knowledge_base,
const Literal ** const literals, const unsigned int literals_size, RuleQueue * const fired_rules);
char *knowledge_base_to_string(const KnowledgeBase * const knowledge_base);

#endif