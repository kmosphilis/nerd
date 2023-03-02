#include <check.h>
#include <stdlib.h>

#include "../src/scene.h"
#include "helper/literal.h"
#include "helper/scene.h"

START_TEST(construct_destruct_test) {
    Scene *scene = NULL;

    scene = scene_constructor();
    ck_assert_scene_empty(scene);
    scene_destructor(&scene);
    ck_assert_ptr_null(scene);

    scene_destructor(&scene);
    ck_assert_ptr_null(scene);

    scene_destructor(NULL);
}
END_TEST

START_TEST(add_test) {
    Scene *scene = scene_constructor();
    Literal *literal1 = literal_constructor("Penguin", 1), *copy;

    literal_copy(&copy, literal1);
    scene_add_literal(scene, &literal1);
    ck_assert_ptr_null(literal1);
    ck_assert_int_eq(scene->size, 1);
    ck_assert_literal_eq(scene->literals[0], copy);
    literal_destructor(&copy);
    ck_assert_scene_notempty(scene);

    Literal *literal2 = literal_constructor("Antarctica", 1);
    literal_copy(&copy, literal2);
    scene_add_literal(scene, &literal2);
    ck_assert_ptr_null(literal2);
    ck_assert_int_eq(scene->size, 2);
    ck_assert_literal_eq(scene->literals[1], copy);
    literal_destructor(&copy);

    Literal *literal3 = literal_constructor("Bird", 1);
    literal_copy(&copy, literal3);
    scene_add_literal(scene, &literal3);
    ck_assert_ptr_null(literal3);
    ck_assert_int_eq(scene->size, 3);
    ck_assert_literal_eq(scene->literals[2], copy);
    literal_destructor(&copy);

    Literal *literal4 = literal_constructor("Fly", 0);
    literal_copy(&copy, literal4);
    scene_add_literal(scene, &literal4);
    ck_assert_ptr_null(literal4);
    ck_assert_int_eq(scene->size, 4);
    ck_assert_literal_eq(scene->literals[3], copy);

    scene_add_literal(scene, NULL);
    ck_assert_int_eq(scene->size, 4);

    scene_add_literal(NULL, &copy);
    ck_assert_int_eq(scene->size, 4);

    scene_add_literal(scene, &copy);
    ck_assert_ptr_nonnull(copy);
    ck_assert_int_ne(scene->size, 5);
    ck_assert_int_eq(scene->size, 4);

    literal_destructor(&copy);

    scene_add_literal(scene, &copy);
    ck_assert_int_ne(scene->size, 5);
    ck_assert_int_eq(scene->size, 4);

    scene_destructor(&scene);
}
END_TEST

START_TEST(index_retrieval_test)  {
    Scene *scene = scene_constructor();
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Antarctica", 1),
    *l3 = literal_constructor("Bird", 1), *l4 = literal_constructor("Fly", 0),
    *l5 = literal_constructor("Fly", 1), *c1, *c2, *c3, *c4;

    literal_copy(&c1, l1);
    literal_copy(&c2, l2);
    literal_copy(&c3, l3);
    literal_copy(&c4, l4);

    scene_add_literal(scene, &l1);
    scene_add_literal(scene, &l2);
    scene_add_literal(scene, &l3);
    scene_add_literal(scene, &l4);

    ck_assert_int_eq(scene_literal_index(scene, c2), 1);
    ck_assert_int_eq(scene_literal_index(scene, c3), 2);
    ck_assert_int_eq(scene_literal_index(scene, l5), -1);
    ck_assert_int_eq(scene_literal_index(scene, NULL), -2);
    ck_assert_int_eq(scene_literal_index(NULL, c2), -2);

    scene_destructor(&scene);
    literal_destructor(&c1);
    literal_destructor(&c2);
    literal_destructor(&c3);
    literal_destructor(&c4);
    literal_destructor(&l5);
}
END_TEST

START_TEST(delete_test)  {
    Scene *scene = scene_constructor();
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Antarctica", 1),
    *l3 = literal_constructor("Bird", 1), *l4 = literal_constructor("Fly", 0), *c1, *c2, *c3, *c4,
    *removed_literal;

    literal_copy(&c1, l1);
    literal_copy(&c2, l2);
    literal_copy(&c3, l3);
    literal_copy(&c4, l4);

    scene_add_literal(scene, &l1);
    scene_add_literal(scene, &l2);
    scene_add_literal(scene, &l3);
    scene_add_literal(scene, &l4);

    int literal_index = scene_literal_index(scene, c1);
    scene_remove_literal(scene, literal_index, NULL);
    ck_assert_int_eq(scene->size, 3);
    ck_assert_literal_ne(scene->literals[literal_index], c1);
    ck_assert_literal_eq(scene->literals[literal_index], c2);

    literal_index = scene_literal_index(scene, c3);
    scene_remove_literal(scene, literal_index, &removed_literal);
    ck_assert_literal_eq(removed_literal, c3);
    ck_assert_int_eq(scene->size, 2);
    ck_assert_literal_ne(scene->literals[literal_index], c3);
    ck_assert_literal_eq(scene->literals[literal_index], c4);
    literal_destructor(&removed_literal);

    scene_remove_literal(scene, 9, NULL);
    ck_assert_int_eq(scene->size, 2);

    ck_assert_ptr_null(removed_literal);
    scene_remove_literal(scene, 9, &removed_literal);
    ck_assert_int_eq(scene->size, 2);
    ck_assert_ptr_null(removed_literal);

    literal_index = scene_literal_index(scene, c4);
    scene_remove_literal(scene, literal_index, &removed_literal);
    ck_assert_literal_eq(removed_literal, c4);
    ck_assert_int_eq(scene->size, 1);
    ck_assert_literal_ne(scene->literals[0], c4);
    ck_assert_literal_eq(scene->literals[0], c2);
    literal_destructor(&removed_literal);

    scene_remove_literal(NULL, 0, NULL);
    ck_assert_int_eq(scene->size, 1);
    ck_assert_literal_eq(scene->literals[0], c2);

    ck_assert_ptr_null(removed_literal);
    scene_remove_literal(NULL, 0, &removed_literal);
    ck_assert_int_eq(scene->size, 1);
    ck_assert_literal_eq(scene->literals[0], c2);
    ck_assert_ptr_null(removed_literal);

    scene_destructor(&scene);
    literal_destructor(&c1);
    literal_destructor(&c2);
    literal_destructor(&c3);
    literal_destructor(&c4);
}
END_TEST

START_TEST(copy_test) {
    Scene *scene1 = scene_constructor(), *scene2 = NULL;
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Antarctica", 1),
    *l3 = literal_constructor("Bird", 1), *l4 = literal_constructor("Fly", 0);

    scene_add_literal(scene1, &l1);
    scene_add_literal(scene1, &l2);
    scene_add_literal(scene1, &l3);
    scene_add_literal(scene1, &l4);

    scene_copy(&scene2, scene1);

    ck_assert_ptr_ne(scene1->literals, scene2->literals);
    ck_assert_scene_eq(scene1, scene2);
    unsigned int i = 0;
    for (i = 0; i < scene1->size; ++i) {
        ck_assert_literal_eq(scene1->literals[i], scene2->literals[i]);
    }

    scene_destructor(&scene1);

    ck_assert_ptr_null(scene1);
    ck_assert_scene_notempty(scene2);

    scene_copy(NULL, scene2);

    scene_copy(&scene1, NULL);

    scene_copy(&scene1, scene2);
    ck_assert_scene_eq(scene1, scene2);

    scene_destructor(&scene1);
    scene_destructor(&scene2);

    scene1 = scene_constructor();
    ck_assert_scene_empty(scene1);
    scene_copy(&scene2, scene1);
    ck_assert_scene_empty(scene2);

    scene_destructor(&scene1);
    scene_destructor(&scene2);

    scene_copy(&scene1, scene2);
    ck_assert_ptr_null(scene2);
    ck_assert_ptr_null(scene1);
}
END_TEST

START_TEST(to_string_test) {
    Scene *scene = scene_constructor();
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Antarctica", 1),
    *l3 = literal_constructor("Bird", 1), *l4 = literal_constructor("Fly", 0);

    scene_add_literal(scene, &l1);
    scene_add_literal(scene, &l2);
    scene_add_literal(scene, &l3);
    scene_add_literal(scene, &l4);

    char *scene_string = scene_to_string(scene);
    ck_assert_str_eq(scene_string, "Scene: [\n\tpenguin,\n\tantarctica,\n\tbird,\n\t-fly\n]");
    free(scene_string);

    scene_string = scene_to_string(NULL);
    ck_assert_pstr_eq(scene_string, NULL);

    scene_destructor(&scene);

    scene = scene_constructor();
    scene_string = scene_to_string(scene);
    ck_assert_str_eq(scene_string, "Scene: [\n]");
    free(scene_string);

    scene_destructor(&scene);
}
END_TEST

START_TEST(combine_test) {
    Scene *scene1 = scene_constructor(), *scene2 = scene_constructor(),
    *expected = scene_constructor(), *result = NULL;
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Antarctica", 1),
    *l3 = literal_constructor("Bird", 1), *l4 = literal_constructor("Fly", 0),
    *l5 = literal_constructor("Albatross", 1), *c1, *c2, *c3, *c4, *c5;

    literal_copy(&c1, l1);
    literal_copy(&c2, l2);
    literal_copy(&c3, l3);
    literal_copy(&c4, l4);
    literal_copy(&c5, l5);

    scene_add_literal(scene1, &l1);
    scene_add_literal(expected, &c1);

    scene_add_literal(scene1, &l2);
    literal_copy(&l2, c2);
    scene_add_literal(scene2, &l2);
    scene_add_literal(expected, &c2);

    scene_add_literal(scene1, &l3);
    literal_copy(&l3, c3);
    scene_add_literal(scene2, &l3);
    scene_add_literal(expected, &c3);

    scene_add_literal(scene1, &l4);
    literal_copy(&l4, c4);
    scene_add_literal(scene2, &l4);
    scene_add_literal(expected, &c4);

    scene_add_literal(scene2, &l5);
    scene_add_literal(expected, &c5);

    ck_assert_int_eq(scene1->size, 4);
    ck_assert_int_eq(scene2->size, 4);

    scene_union(scene1, scene2, &result);
    ck_assert_int_eq(scene1->size, 4);
    ck_assert_int_eq(scene2->size, 4);
    ck_assert_int_eq(result->size, 5);
    ck_assert_scene_eq(result, expected);
    scene_destructor(&result);

    scene_union(NULL, scene2, &result);
    ck_assert_scene_eq(result, scene2);
    scene_destructor(&result);

    scene_union(scene1, NULL, &result);
    ck_assert_scene_eq(result, scene1);
    scene_destructor(&result);

    scene_union(NULL, NULL, &result);
    ck_assert_ptr_null(result);

    scene_union(scene1, scene2, NULL);

    scene_destructor(&scene1);
    scene_destructor(&scene2);
    scene_destructor(&expected);
}
END_TEST

START_TEST(difference_test) {
    Scene *scene1 = scene_constructor(), *scene2 = scene_constructor(),
    *expected = scene_constructor(), *result = NULL;
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Albatross", 1),
    *l3 = literal_constructor("Antarctica", 1), *l4 = literal_constructor("Bird", 1),
    *l5 = literal_constructor("Fly", 0), *c2, *c3, *c4, *c5;

    literal_copy(&c2, l2);
    literal_copy(&c3, l3);
    literal_copy(&c4, l4);
    literal_copy(&c5, l5);

    scene_add_literal(scene1, &l1);

    scene_add_literal(scene2, &l2);
    scene_add_literal(expected, &c2);

    scene_add_literal(scene1, &l3);
    scene_add_literal(scene2, &c3);

    scene_add_literal(scene1, &l4);
    scene_add_literal(scene2, &c4);

    scene_add_literal(scene1, &l5);
    scene_add_literal(scene2, &c5);

    ck_assert_int_eq(scene1->size, 4);
    ck_assert_int_eq(scene2->size, 4);

    scene_difference(scene2, scene1, &result);
    ck_assert_int_eq(scene1->size, 4);
    ck_assert_int_eq(scene2->size, 4);
    ck_assert_scene_eq(result, expected);
    scene_destructor(&result);

    scene_difference(NULL, scene2, &result);
    ck_assert_scene_eq(result, scene2);
    scene_destructor(&result);

    scene_difference(scene1, NULL, &result);
    ck_assert_scene_eq(result, scene1);
    scene_destructor(&result);

    scene_difference(NULL, NULL, &result);
    ck_assert_ptr_null(result);

    scene_difference(scene1, scene2, NULL);

    scene_destructor(&scene1);
    scene_destructor(&scene2);
    scene_destructor(&expected);
    scene_destructor(&result);
}
END_TEST

START_TEST(intersect_test) {
    Scene *scene1 = scene_constructor(), *scene2 = scene_constructor(),
    *scene3 = scene_constructor(), *scene4 = scene_constructor(), *expected1 = scene_constructor(),
    *expected2 = scene_constructor(), *expected3 = scene_constructor(), *result = NULL;
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Albatross", 1),
    *l3 = literal_constructor("Antarctica", 1), *l4 = literal_constructor("Bird", 1),
    *l5 = literal_constructor("Fly", 0), *c1, *c2, *c3, *c4, *c5;

    literal_copy(&c1, l1);
    literal_copy(&c2, l2);
    literal_copy(&c3, l3);
    literal_copy(&c4, l4);
    literal_copy(&c5, l5);

    scene_add_literal(scene1, &l1);
    literal_copy(&l1, c1);
    scene_add_literal(scene3, &l1);
    scene_add_literal(expected2, &c1);

    scene_add_literal(scene2, &l2);
    literal_copy(&l2, c2);
    scene_add_literal(scene3, &l2);
    literal_copy(&l2, c2);
    scene_add_literal(scene4, &l2);
    scene_add_literal(expected3, &c2);

    scene_add_literal(scene1, &l3);
    literal_copy(&l3, c3);
    scene_add_literal(scene2, &l3);
    literal_copy(&l3, c3);
    scene_add_literal(scene3, &l3);
    literal_copy(&l3, c3);
    scene_add_literal(expected1, &l3);
    literal_copy(&l3, c3);
    scene_add_literal(expected2, &l3);
    scene_add_literal(expected3, &c3);

    scene_add_literal(scene1, &l4);
    literal_copy(&l4, c4);
    scene_add_literal(scene2, &l4);
    scene_add_literal(expected1, &c4);

    scene_add_literal(scene1, &l5);
    literal_copy(&l5, c5);
    scene_add_literal(scene2, &l5);
    scene_add_literal(expected1, &c5);

    ck_assert_int_eq(scene1->size, 4);
    ck_assert_int_eq(scene2->size, 4);

    scene_intersect(scene2, scene1, &result);
    ck_assert_int_eq(scene1->size, 4);
    ck_assert_int_eq(scene2->size, 4);
    ck_assert_scene_eq(result, expected1);
    scene_destructor(&result);

    scene_intersect(scene1, scene2, &result);
    ck_assert_scene_eq(result, expected1);
    scene_destructor(&result);

    scene_intersect(scene1, scene3, &result);
    ck_assert_scene_eq(result, expected2);
    scene_destructor(&result);

    scene_intersect(scene2, scene3, &result);
    ck_assert_scene_eq(result, expected3);
    scene_destructor(&result);
    scene_destructor(&scene3);

    scene_intersect(scene2, scene3, &result);
    ck_assert_ptr_null(result);

    scene_intersect(scene3, scene1, &result);
    ck_assert_ptr_null(result);

    scene_intersect(scene1, scene4, &result);
    ck_assert_scene_notempty(scene1);
    ck_assert_scene_notempty(scene4);
    ck_assert_scene_empty(result);
    scene_destructor(&result);

    scene_intersect(NULL, scene2, &result);
    ck_assert_ptr_null(result);

    scene_intersect(scene1, NULL, &result);
    ck_assert_ptr_null(result);

    scene_intersect(NULL, NULL, &result);
    ck_assert_ptr_null(result);

    scene_intersect(scene1, scene2, NULL);

    scene_destructor(&scene1);
    scene_destructor(&scene2);
    scene_destructor(&scene4);
    scene_destructor(&expected1);
    scene_destructor(&expected2);
    scene_destructor(&expected3);
    scene_destructor(&result);
}
END_TEST

START_TEST(opposed_literals_test) {
    Scene *scene1 = scene_constructor(), *scene2 = scene_constructor(),
    *expected1 = scene_constructor(), *expected2 = scene_constructor(), *result = NULL;
    Literal *l1 = literal_constructor("Penguin", 1), *l2 = literal_constructor("Bird", 1),
    *l3 = literal_constructor("Fly", 0), *l4 = literal_constructor("Fly", 1),
    *l5 = literal_constructor("Antarctica", 1), *l6 = literal_constructor("Antarctica", 0), *c1,
    *c2, *c3, *c4, *c6;

    literal_copy(&c1, l1);
    literal_copy(&c2, l2);
    literal_copy(&c3, l3);
    literal_copy(&c4, l4);
    literal_copy(&c6, l6);

    scene_add_literal(scene1, &l1);
    scene_add_literal(scene2, &c1);

    scene_add_literal(scene1, &l2);
    scene_add_literal(scene2, &c2);

    scene_add_literal(scene1, &l3);
    scene_add_literal(expected1, &c3);

    scene_add_literal(scene2, &l4);
    scene_add_literal(expected2, &c4);

    scene_add_literal(scene1, &l5);

    scene_opposed_literals(scene1, scene2, &result);
    ck_assert_scene_eq(expected2, result);
    scene_destructor(&result);

    scene_opposed_literals(scene2, scene1, &result);
    ck_assert_scene_eq(expected1, result);
    scene_destructor(&result);

    scene_add_literal(scene2, &l6);
    scene_add_literal(expected2, &c6);

    scene_opposed_literals(scene1, scene2, &result);
    ck_assert_scene_eq(expected2, result);
    scene_destructor(&result);

    scene_opposed_literals(scene1, NULL, &result);
    ck_assert_ptr_null(result);

    scene_opposed_literals(NULL, scene2, &result);
    ck_assert_ptr_null(result);

    scene_opposed_literals(NULL, NULL, &result);
    ck_assert_ptr_null(result);

    scene_opposed_literals(scene1, scene2, NULL);

    scene_destructor(&scene1);
    scene_destructor(&scene2);
    scene_destructor(&expected1);
    scene_destructor(&expected2);
    scene_destructor(&result);
}
END_TEST

Suite *scene_suite() {
    Suite *suite;
    TCase *create_case, *manipulation_case, *copy_case, *to_string_case;
    suite = suite_create("Scene");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    manipulation_case = tcase_create("Manipulation");
    tcase_add_test(manipulation_case, add_test);
    tcase_add_test(manipulation_case, add_a_copy_test);
    tcase_add_test(manipulation_case, index_retrieval_test);
    tcase_add_test(manipulation_case, delete_test);
    tcase_add_test(manipulation_case, combine_test);
    tcase_add_test(manipulation_case, difference_test);
    tcase_add_test(manipulation_case, intersect_test);
    tcase_add_test(manipulation_case, opposed_literals_test);
    suite_add_tcase(suite, manipulation_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    to_string_case = tcase_create("To string");
    tcase_add_test(to_string_case, to_string_test);
    suite_add_tcase(suite, to_string_case);

    return suite;
}

int main() {
    Suite *suite = scene_suite();
    SRunner *s_runner = srunner_create(suite);

    srunner_set_fork_status(s_runner, CK_NOFORK);
    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
