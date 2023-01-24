#include <check.h>

#include "../../src/scene.h"

#ifndef __SCENE_HELPER_H__
#define __SCENE_HELPER_H__

/**
 * @brief Check two Scenes to determine if they are equal (X == Y).
 *
 * @param X The first Scene to compare.
 * @param Y The second Scene to compare.
 */
#define ck_assert_scene_eq(X, Y) do { \
    const Scene * const s1 = (X); \
    const Scene * const s2 = (Y); \
    ck_assert_ptr_nonnull(s1); \
    ck_assert_ptr_nonnull(s2); \
    ck_assert_ptr_ne(s1->observations, s2->observations); \
    unsigned int i; \
    ck_assert_int_eq(s1->size, s2->size); \
    for (i = 0; i < s1->size; ++i) { \
        ck_assert_literal_eq(&(s1->observations[i]), &(s2->observations[i])); \
    } \
} while (0)

#define _ck_assert_scene_empty(X, OP) do { \
    const Scene * const s = (X); \
    ck_assert_ptr_nonnull(s); \
    _ck_assert_int(s->size, OP, 0); \
    _ck_assert_ptr_null(s->observations, OP); \
} while (0)

/**
 * @brief Check if a Scene is empty.
 *
 * @param X The Scene to check.
 */
#define ck_assert_scene_empty(X) _ck_assert_scene_empty(X, ==)

/**
 * @brief Check if a Scene is not empty.
 *
 * @param X The Scene to check.
 */
#define ck_assert_scene_notempty(X) _ck_assert_scene_empty(X, !=)

#endif
