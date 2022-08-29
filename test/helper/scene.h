#include "../../src/scene.h"

#ifndef __SCENE_HELPER_H__
#define __SCENE_HELPER_H__

void ck_assert_scene_eq(const Scene * const scene1, const Scene * const scene2);
void ck_assert_scene_notempty(const Scene * const scene);
void ck_assert_scene_empty(const Scene * const scene);

#endif