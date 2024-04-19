#ifndef KNOWLEDGE_BASE_H
#define KNOWLEDGE_BASE_H

#include "context.h"
#include "nerd_utils.h"
#include "rule_hypergraph.h"
#include "rule_queue.h"
#include "scene.h"

struct RuleHyperGraph;
typedef struct KnowledgeBase {
  RuleQueue *active;
  float activation_threshold;
  struct RuleHyperGraph *hypergraph;
} KnowledgeBase;

KnowledgeBase *knowledge_base_constructor(const float activation_threshold,
                                          const bool use_backward_chaining);
void knowledge_base_destructor(KnowledgeBase **const knowledge_base);
void knowledge_base_copy(KnowledgeBase **const restrict destination,
                         const KnowledgeBase *const restrict source);
int knowledge_base_add_rule(KnowledgeBase *const knowledge_base,
                            Rule **const rule);
void knowledge_base_create_new_rules(
    KnowledgeBase *const KnowledgeBase, const Scene *const restrict observed,
    const Scene *const restrict inferred, const unsigned int max_body_size,
    const unsigned int max_number_of_rules,
    const Context *const restrict focused_labels);
char *knowledge_base_to_string(const KnowledgeBase *const knowledge_base);
char *knowledge_base_to_prudensjs(const KnowledgeBase *const knowledge_base);

#endif
