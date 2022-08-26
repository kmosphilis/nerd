#include "literal.h"

#ifndef __SCENE_H__
#define __SCENE_H__

typedef struct Scene {
    Literal *observations;
    unsigned int size;
} Scene;

void scene_construct(Scene * const scene);
void scene_destruct(Scene * const scene);
void scene_copy(Scene * const destination, const Scene * const source);
void scene_add_literal(Scene * const scene, const Literal * const literal_to_add);
char *scene_to_string(const Scene * const scene);

#endif