#include <check.h>
#include <stdlib.h>

#include "../src/scene.h"
#include "helper/literal.h"

START_TEST(construct_destruct_test) {
    Scene scene, *scene_ptr = NULL;

    scene_construct(&scene);
    ck_assert_ptr_null(scene.observations);
    ck_assert_int_eq(scene.size, 0);
    scene_destruct(&scene);
    ck_assert_ptr_null(scene.observations);
    ck_assert_int_eq(scene.size, 0);

    scene_construct(scene_ptr);
    ck_assert_ptr_null(scene_ptr);
    scene_destruct(scene_ptr);

    scene_destruct(&scene);
}
END_TEST

START_TEST(add_test) {
    Scene scene, *scene_ptr = NULL;
    Literal literal;

    scene_construct(&scene);
    literal_construct(&literal, "Penguin", 1);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 1);
    ck_assert_literal_eq(&(scene.observations[0]), &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Antarctica", 1);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 2);
    ck_assert_literal_eq(&(scene.observations[1]), &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Bird", 1);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 3);
    ck_assert_literal_eq(&(scene.observations[2]), &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Fly", 0);
    scene_add_literal(&scene, &literal);
    ck_assert_int_eq(scene.size, 4);
    ck_assert_literal_eq(&(scene.observations[3]), &literal);

    scene_add_literal(&scene, NULL);
    ck_assert_int_eq(scene.size, 4);

    scene_add_literal(scene_ptr, &literal);
    ck_assert_ptr_null(scene_ptr);

    literal_destruct(&literal);
    scene_destruct(&scene);
}
END_TEST

START_TEST(copy_test) {
    Scene scene1, scene2, *scene_ptr1 = NULL, *scene_ptr2 = NULL;
    Literal literal;

    scene_construct(&scene1);
    literal_construct(&literal, "Penguin", 1);
    scene_add_literal(&scene1, &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Antarctica", 1);
    scene_add_literal(&scene1, &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Bird", 1);
    scene_add_literal(&scene1, &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Fly", 0);
    scene_add_literal(&scene1, &literal);
    literal_destruct(&literal);

    scene_copy(&scene2, &scene1);

    ck_assert_ptr_ne(scene1.observations, scene2.observations);
    ck_assert_int_eq(scene1.size, scene2.size);

    unsigned int i;
    for (i = 0; i < scene1.size; ++i) {
        ck_assert_ptr_ne(&(scene1.observations[i]), &(scene2.observations[i]));
        ck_assert_literal_eq(&(scene1.observations[i]), &(scene2.observations[i]));
    }

    scene_destruct(&scene1);

    ck_assert_int_ne(scene1.size, scene2.size);
    ck_assert_ptr_null(scene1.observations);
    ck_assert_ptr_nonnull(scene2.observations);

    scene_copy(scene_ptr1, scene_ptr2);
    ck_assert_ptr_null(scene_ptr1);
    ck_assert_ptr_null(scene_ptr2);
    
    scene_ptr2 = &scene2;

    scene_copy(scene_ptr1, scene_ptr2);
    ck_assert_ptr_null(scene_ptr1);
    ck_assert_ptr_nonnull(scene_ptr2);

    scene_copy(scene_ptr2, scene_ptr1);
    ck_assert_ptr_null(scene_ptr1);
    ck_assert_ptr_nonnull(scene_ptr2);

    scene_destruct(&scene2);
}
END_TEST

START_TEST(to_string_test) {
    Scene scene, *scene_ptr = NULL;
    Literal literal;

    scene_construct(&scene);
    literal_construct(&literal, "Penguin", 1);
    scene_add_literal(&scene, &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Antarctica", 1);
    scene_add_literal(&scene, &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Bird", 1);
    scene_add_literal(&scene, &literal);
    literal_destruct(&literal);

    literal_construct(&literal, "Fly", 0);
    scene_add_literal(&scene, &literal);

    literal_destruct(&literal);

    char *scene_string = scene_to_string(&scene);
    ck_assert_str_eq(scene_string, "Scene: [\n\tPenguin,\n\tAntarctica,\n\tBird,\n\t-Fly\n]");
    free(scene_string);

    scene_string = scene_to_string(scene_ptr);
    ck_assert_pstr_eq(scene_string, NULL);

    scene_destruct(&scene);

    scene_string = scene_to_string(&scene);
    ck_assert_str_eq(scene_string, "Scene: [\n]");
    free(scene_string);
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