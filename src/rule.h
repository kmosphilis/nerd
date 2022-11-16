#include "literal.h"
#include "context.h"

#ifndef __RULE_H__
#define __RULE_H__

typedef Context Body;

typedef struct Rule {
    Body body;
    Literal head;
    float weight;
} Rule;

void rule_constructor(Rule * const rule, const unsigned int body_size, Literal ** const body,
const Literal * const head, const float weight);
void rule_destructor(Rule * const rule);
void rule_copy(Rule * const restrict destination, const Rule * const restrict source);
void rule_promote(Rule * const rule, const float amount);
void rule_demote(Rule * const rule, const float amount);
int rule_applicable(const Rule * const rule, const Context * const context);
int rule_concurs(const Rule * const rule, const Context * const context);
int rule_equals(const Rule * const restrict rule1, const Rule * restrict const rule2);
char *rule_to_string(const Rule * const rule);
char *rule_to_prudensjs(const Rule * const rule, const unsigned int rule_number);

#endif