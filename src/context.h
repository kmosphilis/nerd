#include "scene.h"

#ifndef CONTEXT_H
#define CONTEXT_H

typedef Scene Context;

void context_constructor(Context * const context);
void context_destructor(Context * const context);
void context_add_literal(Context * const context, const Literal * const literal);
char *context_to_prudensjs(const Context * const context);

#endif
