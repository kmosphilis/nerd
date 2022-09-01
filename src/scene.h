#include "literal.h"

#ifndef __SCENE_H__
#define __SCENE_H__

typedef struct Scene {
    Literal *observations;
    unsigned int size;
} Scene;

void scene_constructor(Scene * const scene);
void scene_destructor(Scene * const scene);
void scene_copy(Scene * const destination, const Scene * const source);
void scene_add_literal(Scene * const scene, const Literal * const literal_to_add);
void scene_remove_literal(Scene * const scene, const unsigned int literal_index);
int scene_literal_index(const Scene * const scene, const Literal * const literal);
void scene_combine(const Scene * const scene1, const Scene * const2, Scene * const result);
void scene_difference(const Scene * const scene1, const Scene * const scene2, Scene * const result);
char *scene_to_string(const Scene * const scene);

#endif