#include <check.h>

#include "../../src/scene.h"

#ifndef SCENE_TEST_HELPER_H
#define SCENE_TEST_HELPER_H

/**
 * @brief Check two Scenes to determine if they are equal (X == Y).
 *
 * @param X The first Scene to compare.
 * @param Y The second Scene to compare.
 */
#define ck_assert_scene_eq(X, Y)                                               \
  do {                                                                         \
    const Scene *const _s1 = (X);                                              \
    const Scene *const _s2 = (Y);                                              \
    ck_assert_ptr_nonnull(_s1);                                                \
    ck_assert_ptr_nonnull(_s2);                                                \
    unsigned int i;                                                            \
    ck_assert_int_eq(_s1->size, _s2->size);                                    \
    for (i = 0; i < _s1->size; ++i) {                                          \
      ck_assert_literal_eq(_s1->literals[i], _s2->literals[i]);                \
    }                                                                          \
  } while (0)

#define _ck_assert_scene_empty(X, OP)                                          \
  do {                                                                         \
    const Scene *const _s = (X);                                               \
    ck_assert_ptr_nonnull(_s);                                                 \
    _ck_assert_int(_s->size, OP, 0);                                           \
    _ck_assert_ptr_null(_s->literals, OP);                                     \
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
