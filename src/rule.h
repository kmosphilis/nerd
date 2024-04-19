#ifndef RULE_H
#define RULE_H

#include <stdbool.h>

#include "context.h"
#include "literal.h"


typedef Context Body;

typedef struct Rule {
  Body *body;
  Literal *head;
  float weight;
} Rule;

Rule *rule_constructor(const unsigned int body_size, Literal **const body,
                       Literal **const head, const float weight,
                       const bool take_ownership);
void rule_destructor(Rule **const rule);
void rule_copy(Rule **const destination, const Rule *const restrict source);
int rule_took_ownership(const Rule *const rule);
void rule_promote(Rule *const rule, const float amount);
void rule_demote(Rule *const rule, const float amount);
int rule_applicable(const Rule *const rule, const Context *const context);
int rule_concurs(const Rule *const rule, const Context *const context);
int rule_equals(const Rule *const restrict rule1,
                const Rule *restrict const rule2);
char *rule_to_string(const Rule *const rule);
char *rule_to_prudensjs(const Rule *const rule, const unsigned int rule_number);

#endif
