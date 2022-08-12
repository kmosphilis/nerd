#include "../../src/rule_queue.h"

#ifndef __RULE_QUEUE_HELPER_H__
#define __RULE_QUEUE_HELPER_H__

void create_rule_queue(RuleQueue * const rule_queue);
Rule *create_rules();
void destruct_rules(Rule * const rules);
void ck_assert_rule_queue_eq(const RuleQueue * const rule_queue1,
const RuleQueue * const rule_queue2);
void ck_assert_rule_queue_ne(const RuleQueue * const rule_queue1,
const RuleQueue * const rule_queue2);
void ck_assert_rule_queue_notempty(const RuleQueue * const rule_queue);
void ck_assert_rule_queue_empty(const RuleQueue * const rule_queue);

#endif