#include <stdbool.h>

#ifndef LITERAL_H
#define LITERAL_H

#include <pcg_variants.h>

// If defined, this seed will be used across the entire algorithm.
extern pcg32_random_t *global_rng;

typedef struct Literal {
    char *atom;
    bool sign;
} Literal;

Literal *literal_constructor(const char * const atom, const bool sign);
Literal *literal_constructor_from_string(const char * const string);
void literal_destructor(Literal ** const literal);
void literal_copy(Literal ** const destination, const Literal * const restrict source);
void literal_negate(Literal * const literal);
int literal_equals(const Literal * const restrict literal1, const Literal * const restrict literal2);
int literal_opposed(const Literal * const restrict literal1,
const Literal * const restrict literal2);
char *literal_to_string(const Literal * const literal);
char *literal_to_prudensjs(const Literal * const literal);

#endif
