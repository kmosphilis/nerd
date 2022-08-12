#include "rule.h"

#ifndef __RULE_QUEUE_H__
#define __RULE_QUEUE_H__

typedef struct RuleQueue {
    Rule * rules;
    unsigned int length;
} RuleQueue;

void rule_queue_construct(RuleQueue * const rule_queue);
void rule_queue_destruct(RuleQueue * const rule_queue);
void rule_queue_copy(RuleQueue * const destination, const RuleQueue * const source);
void rule_queue_enqueue(RuleQueue * const rule_queue, const Rule * const rule);
void rule_queue_dequeue(RuleQueue * const rule_queue, Rule * dequeued_rule);
char *rule_queue_to_string(const RuleQueue * const rule_queue);

#endif