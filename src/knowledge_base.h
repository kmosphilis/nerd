#include "int_vector.h"
#include "rule_queue.h"
#include "scene.h"
#include "context.h"

#ifndef __KNOWLEDGE_BASE_H__
#define __KNOWLEDGE_BASE_H__

typedef struct KnowledgeBase {
    RuleQueue active, inactive;
    float activation_threshold;
} KnowledgeBase;

typedef enum RuleType {
    ACTIVE,
    INACTIVE
} RuleType;

void knowledge_base_constructor(KnowledgeBase * const knolwedge_base,
const float activation_threshold);
void knowledge_base_destructor(KnowledgeBase * const knowledge_base);
void knowledge_base_copy(KnowledgeBase * const destination, const KnowledgeBase * const source);
void knowledge_base_add_rule(KnowledgeBase * const knowledge_base, const Rule * const rule);
void knowledge_base_create_new_rules(KnowledgeBase * const KnowledgeBase,
const Scene * const observed, const Scene * const inferred, const unsigned int max_body_size,
const unsigned int max_number_of_rules);
void knowledge_base_applicable_rules(const KnowledgeBase * restrict knowledge_base,
const Context * restrict context, IntVector * restrict applicable_active_rules,
IntVector * restrict applicable_inactive_rules);
void knowledge_base_concurring_rules(const KnowledgeBase * restrict knowledge_base,
const Context * restrict context, IntVector * restrict concurring_active_rules,
IntVector * restrict concurring_inactive_rules);
void knowledge_base_promote_rules(KnowledgeBase * const knowledge_base,
const RuleQueue * const rules_to_promote, const float promotion_rate);
void knowledge_base_demote_rules(KnowledgeBase * const knowledge_base,
const RuleQueue * const rules_to_demote, const float demotion_rate);
void knowledge_base_promote_rule(KnowledgeBase * restrict knowledge_base, const RuleType type,
const unsigned int rule_index, const float promotion_weight);
void knowledge_base_demote_rule(KnowledgeBase * restrict knowledge_base, const RuleType type,
const unsigned int rule_index, const float demotion_weight);
char *knowledge_base_to_string(const KnowledgeBase * const knowledge_base);
char *knowledge_base_to_prudensjs(const KnowledgeBase * const knowledge_base);

#endif