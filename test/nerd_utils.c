#include <check.h>
#include <stdlib.h>

#include "../src/nerd_utils.h"
#include "helper/nerd_utils.h"

START_TEST(trim_test) {
    const char *expected1 = "test", *expected2 = "test 1", *expected3 = "test     1";
    char *str = strdup("test");
    char *trimmed = trim(str);
    ck_assert_str_eq(trimmed, str);
    ck_assert_str_eq(trimmed, expected1);
    free(str);
    free(trimmed);

    str = strdup(" test");
    trimmed = trim(str);
    ck_assert_str_ne(trimmed, str);
    ck_assert_str_eq(trimmed, expected1);
    free(str);
    free(trimmed);

    str = strdup ("     test");
    trimmed = trim(str);
    ck_assert_str_ne(trimmed, str);
    ck_assert_str_eq(trimmed, expected1);
    free(str);
    free(trimmed);

    str = strdup("test ");
    trimmed = trim(str);
    ck_assert_str_ne(trimmed, str);
    ck_assert_str_eq(trimmed, expected1);
    free(str);
    free(trimmed);

    str = strdup("test     ");
    trimmed = trim(str);
    ck_assert_str_ne(trimmed, str);
    ck_assert_str_eq(trimmed, expected1);
    free(str);
    free(trimmed);

    str = strdup("     test     ");
    trimmed = trim(str);
    ck_assert_str_ne(trimmed, str);
    ck_assert_str_eq(trimmed, expected1);
    free(str);
    free(trimmed);

    str = strdup("test 1");
    trimmed = trim(str);
    ck_assert_str_eq(trimmed, str);
    ck_assert_str_eq(trimmed, expected2);
    free(str);
    free(trimmed);

    str = strdup("     test 1     ");
    trimmed = trim(str);
    ck_assert_str_ne(trimmed, str);
    ck_assert_str_eq(trimmed, expected2);
    free(str);
    free(trimmed);

    str = strdup("     test     1     ");
    trimmed = trim(str);
    ck_assert_str_ne(trimmed, str);
    ck_assert_str_eq(trimmed, expected3);
    free(str);
    free(trimmed);
}
END_TEST

START_TEST(IV_construct_destruct_test) {
    IntVector *int_vector = NULL;

    int_vector = int_vector_constructor();
    ck_assert_int_vector_empty(int_vector);

    int_vector_destructor(&int_vector);
    ck_assert_ptr_null(int_vector);

    int_vector_destructor(&int_vector);
    ck_assert_ptr_null(int_vector);

    int_vector_destructor(NULL);
}
END_TEST

START_TEST(IV_resize_test) {
    IntVector *int_vector = NULL;

    int_vector = int_vector_constructor();

    int_vector_push(int_vector, 5);
    int_vector_push(int_vector, 9);
    int_vector_push(int_vector, 15);

    ck_assert_int_eq(int_vector->size, 3);
    int_vector_resize(int_vector, 7);
    ck_assert_int_eq(int_vector->size, 7);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 9);
    ck_assert_int_eq(int_vector->items[2], 15);
    unsigned int i;
    for (i = 3; i < int_vector->size; ++i) {
        ck_assert_int_eq(int_vector->items[i], 0);
    }

    int old_item = int_vector->items[2];
    int_vector_resize(int_vector, 2);
    ck_assert_int_eq(int_vector->size, 2);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 9);
    ck_assert_int_eq(int_vector_get(int_vector, 2), 2);

    int_vector_resize(int_vector, 3);
    ck_assert_int_ne(int_vector->items[2], old_item);
    ck_assert_int_eq(int_vector->items[2], 0);

    int_vector_destructor(&int_vector);

    int_vector_resize(int_vector, 2);
    ck_assert_ptr_null(int_vector);
}
END_TEST

START_TEST(IV_push_test) {
    IntVector *int_vector = NULL;

    int_vector = int_vector_constructor();

    int_vector_push(int_vector, 5);
    ck_assert_int_vector_notempty(int_vector);
    ck_assert_int_eq(int_vector->size, 1);
    ck_assert_int_eq(int_vector->items[0], 5);

    int_vector_push(int_vector, 9);
    ck_assert_int_eq(int_vector->size, 2);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 9);

    int_vector_destructor(&int_vector);
    int_vector = int_vector_constructor();

    int_vector_push(int_vector, 15);
    ck_assert_int_eq(int_vector->size, 1);
    ck_assert_int_eq(int_vector->items[0], 15);

    int_vector_destructor(&int_vector);

    int_vector_push(NULL, 5);
}
END_TEST

START_TEST(IV_insert_test) {
    IntVector *int_vector = int_vector_constructor();

    int_vector_push(int_vector, 5);
    int_vector_push(int_vector, 9);
    int_vector_push(int_vector, 15);
    int_vector_push(int_vector, 20);
    ck_assert_int_eq(int_vector->size, 4);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 9);
    ck_assert_int_eq(int_vector->items[2], 15);
    ck_assert_int_eq(int_vector->items[3], 20);
    int_vector_insert(int_vector, 1, 50);
    ck_assert_int_eq(int_vector->size, 5);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 50);
    ck_assert_int_eq(int_vector->items[2], 9);
    ck_assert_int_eq(int_vector->items[3], 15);
    ck_assert_int_eq(int_vector->items[4], 20);

    int_vector_insert(int_vector, 3, 500);
    ck_assert_int_eq(int_vector->size, 6);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 50);
    ck_assert_int_eq(int_vector->items[2], 9);
    ck_assert_int_eq(int_vector->items[3], 500);
    ck_assert_int_eq(int_vector->items[4], 15);
    ck_assert_int_eq(int_vector->items[5], 20);

    int_vector_insert(int_vector, 5, 80);
    ck_assert_int_eq(int_vector->size, 7);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 50);
    ck_assert_int_eq(int_vector->items[2], 9);
    ck_assert_int_eq(int_vector->items[3], 500);
    ck_assert_int_eq(int_vector->items[4], 15);
    ck_assert_int_eq(int_vector->items[5], 80);
    ck_assert_int_eq(int_vector->items[6], 20);

    int_vector_insert(int_vector, 7, 9001);
    ck_assert_int_eq(int_vector->size, 8);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 50);
    ck_assert_int_eq(int_vector->items[2], 9);
    ck_assert_int_eq(int_vector->items[3], 500);
    ck_assert_int_eq(int_vector->items[4], 15);
    ck_assert_int_eq(int_vector->items[5], 80);
    ck_assert_int_eq(int_vector->items[6], 20);
    ck_assert_int_eq(int_vector->items[7], 9001);

    int_vector_insert(int_vector, 9, 42);
    ck_assert_int_eq(int_vector->size, 8);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 50);
    ck_assert_int_eq(int_vector->items[2], 9);
    ck_assert_int_eq(int_vector->items[3], 500);
    ck_assert_int_eq(int_vector->items[4], 15);
    ck_assert_int_eq(int_vector->items[5], 80);
    ck_assert_int_eq(int_vector->items[6], 20);
    ck_assert_int_eq(int_vector->items[7], 9001);

    int_vector_insert(NULL, 0, 50);
    ck_assert_int_eq(int_vector->size, 8);

    int_vector_destructor(&int_vector);
    ck_assert_ptr_null(int_vector);

    int_vector_insert(int_vector, 0, 800);
    ck_assert_ptr_null(int_vector);

    int_vector = int_vector_constructor();
    int_vector_insert(int_vector, 0, 800);
    ck_assert_int_eq(int_vector->size, 1);
    ck_assert_int_eq(int_vector->items[0], 800);

    int_vector_destructor(&int_vector);
}
END_TEST

START_TEST(IV_delete_test) {
    IntVector *int_vector = NULL;

    int_vector = int_vector_constructor();

    int_vector_push(int_vector, 5);
    int_vector_push(int_vector, 9);
    int_vector_push(int_vector, 15);
    int_vector_push(int_vector, 20);
    ck_assert_int_eq(int_vector->size, 4);
    ck_assert_int_eq(int_vector->items[0], 5);
    ck_assert_int_eq(int_vector->items[1], 9);
    ck_assert_int_eq(int_vector->items[2], 15);
    ck_assert_int_eq(int_vector->items[3], 20);

    int_vector_delete(int_vector, 0);
    ck_assert_int_eq(int_vector->size, 3);
    ck_assert_int_eq(int_vector->items[0], 9);
    ck_assert_int_eq(int_vector->items[1], 15);
    ck_assert_int_eq(int_vector->items[2], 20);

    int_vector_delete(int_vector, 1);
    ck_assert_int_eq(int_vector->size, 2);
    ck_assert_int_eq(int_vector->items[0], 9);
    ck_assert_int_eq(int_vector->items[1], 20);

    int_vector_delete(int_vector, 1);
    ck_assert_int_eq(int_vector->size, 1);
    ck_assert_int_eq(int_vector->items[0], 9);

    int_vector_delete(int_vector, 0);
    ck_assert_int_vector_empty(int_vector);

    int_vector_destructor(&int_vector);
}
END_TEST

START_TEST(IV_get_test) {
    IntVector *int_vector = NULL;

    int_vector = int_vector_constructor();

    int_vector_push(int_vector, 5);
    int_vector_push(int_vector, 9);
    int_vector_push(int_vector, 15);
    ck_assert_int_eq(int_vector_get(int_vector, 0), int_vector->items[0]);
    ck_assert_int_eq(int_vector_get(int_vector, 1), int_vector->items[1]);
    ck_assert_int_eq(int_vector_get(int_vector, 2), int_vector->items[2]);
    ck_assert_int_eq(int_vector_get(int_vector, 3), 3);

    int_vector_push(int_vector, -20);
    ck_assert_int_eq(int_vector_get(int_vector, 3), int_vector->items[3]);
    ck_assert_int_eq(int_vector_get(int_vector, 4), 4);

    int previous_value = int_vector->items[1];
    int_vector_delete(int_vector, 1);
    ck_assert_int_eq(int_vector_get(int_vector, 1), int_vector->items[1]);
    ck_assert_int_ne(int_vector_get(int_vector, 1), previous_value);
    int_vector_destructor(&int_vector);

    ck_assert_ptr_null(int_vector);
    ck_assert_int_eq(int_vector_get(int_vector, 8), 8);
    ck_assert_ptr_null(int_vector);
}
END_TEST

START_TEST(IV_set_test) {
    IntVector *int_vector = NULL;

    int_vector = int_vector_constructor();

    int_vector_push(int_vector, 5);
    int_vector_push(int_vector, 9);
    int_vector_push(int_vector, 15);

    int previous_value = int_vector_get(int_vector, 2), old_size = int_vector->size;
    ck_assert_int_eq(int_vector_set(int_vector, 2, -31), 1);
    ck_assert_int_ne(int_vector_get(int_vector, 2), previous_value);
    ck_assert_int_eq(int_vector->size, old_size);

    ck_assert_int_eq(int_vector_set(int_vector, 4, 80), 0);
    ck_assert_int_eq(int_vector->size, old_size);
    int_vector_destructor(&int_vector);

    ck_assert_ptr_null(int_vector);
    ck_assert_int_eq(int_vector_set(int_vector, 2, 55), -1);
    ck_assert_ptr_null(int_vector);
}
END_TEST

START_TEST(IV_copy_test) {
    IntVector *int_vec1 = NULL, *int_vec2 = NULL;

    int_vec1 = int_vector_constructor();

    int_vector_push(int_vec1, 5);
    int_vector_push(int_vec1, 9);
    int_vector_push(int_vec1, 15);

    int_vector_copy(&int_vec2, int_vec1);
    ck_assert_int_vector_eq(int_vec1, int_vec2);
    int_vector_destructor(&int_vec2);

    int_vector_copy(NULL, int_vec1);
    ck_assert_ptr_null(int_vec2);
    ck_assert_int_vector_notempty(int_vec1);
    int_vector_destructor(&int_vec1);

    int_vector_copy(&int_vec1, NULL);
    ck_assert_ptr_null(int_vec2);
    ck_assert_ptr_null(int_vec1);
}
END_TEST

Suite *functions_suite() {
    Suite *suite = suite_create("Functions");
    TCase *trim_function_case = tcase_create("Trim");

    tcase_add_test(trim_function_case, trim_test);
    suite_add_tcase(suite, trim_function_case);

    return suite;
}

Suite *int_vector_suite() {
    Suite *suite;
    TCase *create_case, *copy_case, *manipulation_case;
    suite = suite_create("Int Vector");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, IV_construct_destruct_test);
    suite_add_tcase(suite, create_case);

    manipulation_case = tcase_create("Manipulation");
    tcase_add_test(manipulation_case, IV_resize_test);
    tcase_add_test(manipulation_case, IV_push_test);
    tcase_add_test(manipulation_case, IV_insert_test);
    tcase_add_test(manipulation_case, IV_delete_test);
    tcase_add_test(manipulation_case, IV_get_test);
    tcase_add_test(manipulation_case, IV_set_test);
    suite_add_tcase(suite, manipulation_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, IV_copy_test);
    suite_add_tcase(suite, copy_case);

    return suite;
}

int main() {
    Suite *suite = int_vector_suite();
    SRunner *s_runner = srunner_create(suite);

    srunner_add_suite(s_runner, functions_suite());
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
