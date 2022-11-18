#include "scene.h"

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

typedef Scene Context;

Context *context_constructor();
void context_destructor(Context **context);
void context_add_literal(Context * const context, const Literal * const literal);
char *context_to_prudensjs(const Context * const context);

#endif