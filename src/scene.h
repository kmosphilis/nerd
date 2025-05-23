#include <stdbool.h>
#include <stdlib.h>

#include "literal.h"

#ifndef SCENE_H
#define SCENE_H

typedef struct Scene {
  Literal **literals;
  size_t size;
} Scene;

Scene *scene_constructor(const bool take_ownership);
void scene_destructor(Scene **const scene);
void scene_copy(Scene **const destination, const Scene *const restrict source);
int scene_is_taking_ownership(const Scene *const scene);
void scene_add_literal(Scene *const scene, Literal **const literal_to_add);
void scene_remove_literal(Scene *const scene, const unsigned int literal_index,
                          Literal **const removed_literal);
int scene_literal_index(const Scene *const scene, const Literal *const literal);
void scene_union(const Scene *const restrict scene1,
                 const Scene *const restrict const2,
                 Scene **const restrict result);
void scene_difference(const Scene *const restrict scene1,
                      const Scene *const restrict scene2,
                      Scene **const restrict result);
void scene_intersect(const Scene *const restrict scene1,
                     const Scene *const restrict scene2,
                     Scene **const restrict result);
int scene_is_subset(const Scene *const restrict scene1,
                    const Scene *const restrict scene2);
int scene_number_of_similar_literals(const Scene *const restrict scene1,
                                     const Scene *const restrict scene2);
void scene_opposed_literals(const Scene *const restrict scene1,
                            const Scene *const restrict scene2,
                            Scene **const restrict result,
                            const Scene *const restrict oppositions);
char *scene_to_string(const Scene *const scene);

#endif
