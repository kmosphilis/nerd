#include "../../src/rule.h"

#ifndef __RULE_HELPER_H__
#define __RULE_HELPER_H__
#define RULES_TO_CREATE 3

void ck_assert_rule_eq(const Rule * const rule1, const Rule * const rule2);
void ck_assert_rule_notempty(const Rule * const rule);
void ck_assert_rule_empty(const Rule * const rule);

#endif