#ifndef __LITERAL__
#define __LITERAL__

typedef struct Literal {
    char *atom;
    int sign;
} Literal;

void literal_construct(Literal *literal, const char * const atom, const int sign);
void literal_destruct(Literal * literal);
void literal_copy(Literal * const destination, const Literal  * const source);
char *literal_to_string(const Literal * const literal);
void literal_negate(Literal * const literal);

#endif