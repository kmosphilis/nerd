#include <check.h>

#include "scene.h"
#include "literal.h"
/**
 * @brief Check two Scenes to determine if they are equal (scene1 == scene2).
 * 
 * @param scene1 The first Scene to compare.
 * @param scene2 The second Scene to compare.
 */
void ck_assert_scene_eq(const Scene * const scene1, const Scene * const scene2) {
    ck_assert_int_eq(scene1->size, scene2->size);
    unsigned int i;
    for (i = 0; i < scene1->size; ++i) {
        ck_assert_literal_eq(&(scene1->observations[i]), &(scene2->observations[i]));
    }
}

/**
 * @brief Check if a Scene is not empty.
 * 
 * @param scene The Scene to check.
 */
void ck_assert_scene_notempty(const Scene * const scene) {
    ck_assert_int_ne(scene->size, 0);
    ck_assert_ptr_nonnull(scene->observations);
}

/**
 * @brief Check if a Scene is empty.
 * 
 * @param scene The Scene to check.
 */
void ck_assert_scene_empty(const Scene * const scene) {
    ck_assert_int_eq(scene->size, 0);
    ck_assert_ptr_null(scene->observations);
}