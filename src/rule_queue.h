#ifndef RULE_QUEUE_H
#define RULE_QUEUE_H

#include <stdbool.h>
#include <stdio.h>

#include "context.h"
#include "nerd_utils.h"
#include "rule.h"

typedef struct RuleQueue {
  Rule **rules;
  size_t length;
} RuleQueue;

RuleQueue *rule_queue_constructor(const bool take_ownership);
void rule_queue_destructor(RuleQueue **const rule_queue);
void rule_queue_copy(RuleQueue **const destination,
                     const RuleQueue *const source);
int rule_queue_is_taking_ownership(const RuleQueue *const rule_queue);
void rule_queue_enqueue(RuleQueue *const rule_queue, Rule **const rule);
void rule_queue_dequeue(RuleQueue *const rule_queue,
                        Rule **const dequeued_rule);
int rule_queue_find(const RuleQueue *const rule_queue, const Rule *const rule);
void rule_queue_remove_rule(RuleQueue *const rule_queue, const int rule_index,
                            Rule **const removed_rule);
void rule_queue_find_applicable_rules(const RuleQueue *const rule_queue,
                                      const Context *const context,
                                      IntVector **const rule_indices);
void rule_queue_find_concurring_rules(const RuleQueue *const rule_queue,
                                      const Context *const context,
                                      IntVector **const rule_indices);
char *rule_queue_to_string(const RuleQueue *const rule_queue);

#endif
