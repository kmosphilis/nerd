#include <stdbool.h>

#include "scene.h"

#ifndef CONTEXT_H
#define CONTEXT_H

typedef Scene Context;

Context *context_constructor(const bool take_ownership);
void context_destructor(Context **const context);
void context_add_literal(Context *const context,
                         Literal **const literal_to_add);
char *context_to_prudensjs(const Context *const context);

#endif
