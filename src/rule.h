#include "literal.h"

#ifndef __RULE_H__
#define __RULE_H__

typedef struct Rule {
    unsigned int body_size;
    Literal *body;
    Literal head;
    float weight;
} Rule;

void rule_construct(Rule * const rule, const int body_size, Literal ** const body,
    const Literal * const head, const float weight);
void rule_destruct(Rule * const rule);
void rule_copy(Rule * const destination, const Rule * const source);
void rule_promote(Rule * const rule, const float amount);
void rule_demote(Rule * const rule, const float amount);
char *rule_to_string(const Rule * const rule);

#endif