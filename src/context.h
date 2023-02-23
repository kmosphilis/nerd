#include "scene.h"

#ifndef CONTEXT_H
#define CONTEXT_H

typedef Scene Context;

Context *context_constructor();
void context_destructor(Context ** const context);
void context_add_literal(Context * const context, Literal ** const literal);
char *context_to_prudensjs(const Context * const context);

#endif
