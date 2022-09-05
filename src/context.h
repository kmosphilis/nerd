#include "scene.h"

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

typedef Scene Context;

void context_constructor(Context * const context);
void context_destructor(Context * const context);
void context_add_literal(Context * const context, const Literal * const literal);

#endif