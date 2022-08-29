#include <check.h>
#include <stdlib.h>

#include "../src/scene.h"
#include "helper/literal.h"
#include "helper/scene.h"

START_TEST(construct_destruct_test) {
    Scene scene, *scene_ptr = NULL;

    scene_constructor(&scene);
    ck_assert_scene_empty(&scene);
    scene_destructor(&scene);
    ck_assert_scene_empty(&scene);

    scene_constructor(scene_ptr);
    ck_assert_ptr_null(scene_ptr);
    scene_destructor(scene_ptr);

    scene_destructor(&scene);
}
END_TEST

START_TEST(add_test) {
    Scene scene, *scene_ptr = NULL;
    Literal literal;

    scene_constructor(&scene);
    literal_constructor(&literal, "Penguin", 1);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 1);
    ck_assert_literal_eq(&(scene.observations[0]), &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Antarctica", 1);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 2);
    ck_assert_literal_eq(&(scene.observations[1]), &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Bird", 1);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 3);
    ck_assert_literal_eq(&(scene.observations[2]), &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Fly", 0);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 4);
    ck_assert_literal_eq(&(scene.observations[3]), &literal);

    scene_add_literal(&scene, NULL);
    ck_assert_int_eq(scene.size, 4);

    scene_add_literal(scene_ptr, &literal);
    ck_assert_ptr_null(scene_ptr);

    literal_destructor(&literal);
    scene_destructor(&scene);
}
END_TEST

START_TEST(copy_test) {
    Scene scene1, scene2, *scene_ptr1 = NULL, *scene_ptr2 = NULL;
    Literal literal;

    scene_constructor(&scene1);
    literal_constructor(&literal, "Penguin", 1);
    scene_add_literal(&scene1, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Antarctica", 1);
    scene_add_literal(&scene1, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Bird", 1);
    scene_add_literal(&scene1, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Fly", 0);
    scene_add_literal(&scene1, &literal);
    literal_destructor(&literal);

    scene_copy(&scene2, &scene1);

    ck_assert_ptr_ne(scene1.observations, scene2.observations);
    ck_assert_scene_eq(&scene1, &scene2);

    scene_destructor(&scene1);

    ck_assert_int_ne(scene1.size, scene2.size);
    ck_assert_scene_empty(&scene1);
    ck_assert_scene_notempty(&scene2);

    scene_copy(scene_ptr1, scene_ptr2);
    ck_assert_ptr_null(scene_ptr1);
    ck_assert_ptr_null(scene_ptr2);
    
    scene_ptr2 = &scene2;

    scene_copy(scene_ptr1, scene_ptr2);
    ck_assert_ptr_null(scene_ptr1);
    ck_assert_scene_notempty(scene_ptr2);

    scene_copy(scene_ptr2, scene_ptr1);
    ck_assert_ptr_null(scene_ptr1);
    ck_assert_scene_notempty(scene_ptr2);

    scene_destructor(&scene2);
}
END_TEST

START_TEST(to_string_test) {
    Scene scene, *scene_ptr = NULL;
    Literal literal;

    scene_constructor(&scene);
    literal_constructor(&literal, "Penguin", 1);
    scene_add_literal(&scene, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Antarctica", 1);
    scene_add_literal(&scene, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Bird", 1);
    scene_add_literal(&scene, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Fly", 0);
    scene_add_literal(&scene, &literal);

    literal_destructor(&literal);

    char *scene_string = scene_to_string(&scene);
    ck_assert_str_eq(scene_string, "Scene: [\n\tPenguin,\n\tAntarctica,\n\tBird,\n\t-Fly\n]");
    free(scene_string);

    scene_string = scene_to_string(scene_ptr);
    ck_assert_pstr_eq(scene_string, NULL);

    scene_destructor(&scene);

    scene_string = scene_to_string(&scene);
    ck_assert_str_eq(scene_string, "Scene: [\n]");
    free(scene_string);
}
END_TEST

START_TEST(combine_test) {
    Scene scene1, scene2, expected, result, *scene_ptr = NULL;
    Literal literal;

    scene_constructor(&scene1);
    scene_constructor(&scene2);
    scene_constructor(&expected);
    scene_constructor(&result);

    literal_constructor(&literal, "Penguin", 1);
    scene_add_literal(&scene1, &literal);
    scene_add_literal(&expected, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Antarctica", 1);
    scene_add_literal(&scene1, &literal);
    scene_add_literal(&scene2, &literal);
    scene_add_literal(&expected, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Bird", 1);
    scene_add_literal(&scene1, &literal);
    scene_add_literal(&scene2, &literal);
    scene_add_literal(&expected, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Fly", 0);
    scene_add_literal(&scene1, &literal);
    scene_add_literal(&scene2, &literal);
    scene_add_literal(&expected, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Albatross", 1);
    scene_add_literal(&scene2, &literal);
    scene_add_literal(&expected, &literal);
    literal_destructor(&literal);

    scene_combine(&scene1, &scene2, &result);

    ck_assert_int_eq(scene1.size, 4);
    ck_assert_int_eq(scene2.size, 4);
    ck_assert_scene_eq(&result, &expected);
    scene_destructor(&result);

    scene_combine(NULL, &scene2, &result);
    ck_assert_scene_eq(&result, &scene2);
    scene_destructor(&result);

    scene_combine(&scene1, NULL, &result);
    ck_assert_scene_eq(&result, &scene1);
    scene_destructor(&result);

    scene_combine(NULL, NULL, &result);
    ck_assert_scene_empty(&result);
    
    scene_combine(&scene1, &scene2, scene_ptr);
    ck_assert_ptr_null(scene_ptr);

    scene_destructor(&scene1);
    scene_destructor(&scene2);
    scene_destructor(&expected);
}
END_TEST

START_TEST(difference_test) {
    Scene scene1, scene2, expected, result, *scene_ptr = NULL;
    Literal literal;

    scene_constructor(&scene1);
    scene_constructor(&scene2);
    scene_constructor(&expected);
    scene_constructor(&result);

    literal_constructor(&literal, "Penguin", 1);
    scene_add_literal(&scene1, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Albatross", 1);
    scene_add_literal(&scene2, &literal);
    scene_add_literal(&expected, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Antarctica", 1);
    scene_add_literal(&scene1, &literal);
    scene_add_literal(&scene2, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Bird", 1);
    scene_add_literal(&scene1, &literal);
    scene_add_literal(&scene2, &literal);
    literal_destructor(&literal);

    literal_constructor(&literal, "Fly", 0);
    scene_add_literal(&scene1, &literal);
    scene_add_literal(&scene2, &literal);
    literal_destructor(&literal);

    scene_difference(&scene2, &scene1, &result);

    ck_assert_int_eq(scene1.size, 4);
    ck_assert_int_eq(scene2.size, 4);
    ck_assert_scene_eq(&result, &expected);
    scene_destructor(&result);

    scene_difference(NULL, &scene2, &result);
    ck_assert_scene_eq(&result, &scene2);
    scene_destructor(&result);

    scene_difference(&scene1, NULL, &result);
    ck_assert_scene_eq(&result, &scene1);
    scene_destructor(&result);

    scene_difference(NULL, NULL, &result);
    ck_assert_scene_empty(&result);
    
    scene_difference(&scene1, &scene2, scene_ptr);
    ck_assert_ptr_null(scene_ptr);

    scene_destructor(&scene1);
    scene_destructor(&scene2);
    scene_destructor(&expected);
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
    tcase_add_test(manipulation_case, combine_test);
    tcase_add_test(manipulation_case, difference_test);
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
    Suite* suite = scene_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}