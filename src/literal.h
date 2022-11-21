#include <stdint.h>

#ifndef __LITERAL_H__
#define __LITERAL_H__

typedef struct Literal {
    char *atom;
    uint_fast8_t sign;
} Literal;

Literal * literal_constructor(const char * const atom, const uint_fast8_t sign);
void literal_destructor(Literal ** const literal);
void literal_copy(Literal ** const restrict destination, const Literal * const restrict source);
void literal_negate(Literal * const literal);
int literal_equals(const Literal * const restrict literal1, const Literal * const restrict literal2);
int literal_opposed(const Literal * const restrict literal1,
const Literal * const restrict literal2);
char *literal_to_string(const Literal * const literal);
char *literal_to_prudensjs(const Literal * const literal);

#endif