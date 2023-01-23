#include <check.h>
#include <stdlib.h>

#include "../src/int_vector.h"
#include "helper/int_vector.h"

START_TEST(construct_destruct_test) {
    IntVector int_vector, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);
    ck_assert_int_vector_empty(&int_vector);

    int_vector_destructor(&int_vector);
    ck_assert_int_vector_empty(&int_vector);
    int_vector_destructor(&int_vector);
    ck_assert_int_vector_empty(&int_vector);

    int_vector_constructor(int_vector_ptr);
    ck_assert_ptr_null(int_vector_ptr);

    int_vector_destructor(int_vector_ptr);
    ck_assert_ptr_null(int_vector_ptr);
}
END_TEST

START_TEST(resize_test) {
    IntVector int_vector, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);

    int_vector_push(&int_vector, 5);
    int_vector_push(&int_vector, 9);
    int_vector_push(&int_vector, 15);

    ck_assert_int_eq(int_vector.size, 3);
    int_vector_resize(&int_vector, 7);
    ck_assert_int_eq(int_vector.size, 7);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 9);
    ck_assert_int_eq(int_vector.items[2], 15);
    unsigned int i;
    for (i = 3; i < int_vector.size; ++i) {
        ck_assert_int_eq(int_vector.items[i], 0);
    }

    int old_item = int_vector.items[2];
    int_vector_resize(&int_vector, 2);
    ck_assert_int_eq(int_vector.size, 2);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 9);
    ck_assert_int_eq(int_vector_get(&int_vector, 2), 2);

    int_vector_resize(&int_vector, 3);
    ck_assert_int_ne(int_vector.items[2], old_item);
    ck_assert_int_eq(int_vector.items[2], 0);

    int_vector_resize(int_vector_ptr, 99);
    ck_assert_ptr_null(int_vector_ptr);

    int_vector_destructor(&int_vector);
}
END_TEST

START_TEST(push_test) {
    IntVector int_vector, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);

    int_vector_push(&int_vector, 5);
    ck_assert_int_vector_notempty(&int_vector);
    ck_assert_int_eq(int_vector.size, 1);
    ck_assert_int_eq(int_vector.items[0], 5);

    int_vector_push(&int_vector, 9);
    ck_assert_int_eq(int_vector.size, 2);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 9);

    int_vector_destructor(&int_vector);

    int_vector_push(&int_vector, 15);
    ck_assert_int_eq(int_vector.size, 1);
    ck_assert_int_eq(int_vector.items[0], 15);

    int_vector_destructor(&int_vector);

    int_vector_push(int_vector_ptr, 5);
    ck_assert_ptr_null(int_vector_ptr);
}
END_TEST

START_TEST(insert_test) {
    IntVector int_vector, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);
    
    int_vector_push(&int_vector, 5);
    int_vector_push(&int_vector, 9);
    int_vector_push(&int_vector, 15);
    int_vector_push(&int_vector, 20);
    ck_assert_int_eq(int_vector.size, 4);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 9);
    ck_assert_int_eq(int_vector.items[2], 15);
    ck_assert_int_eq(int_vector.items[3], 20);

    int_vector_insert(&int_vector, 1, 50);
    ck_assert_int_eq(int_vector.size, 5);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 50);
    ck_assert_int_eq(int_vector.items[2], 9);
    ck_assert_int_eq(int_vector.items[3], 15);
    ck_assert_int_eq(int_vector.items[4], 20);

    int_vector_insert(&int_vector, 3, 500);
    ck_assert_int_eq(int_vector.size, 6);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 50);
    ck_assert_int_eq(int_vector.items[2], 9);
    ck_assert_int_eq(int_vector.items[3], 500);
    ck_assert_int_eq(int_vector.items[4], 15);
    ck_assert_int_eq(int_vector.items[5], 20);

    int_vector_insert(&int_vector, 5, 80);
    ck_assert_int_eq(int_vector.size, 7);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 50);
    ck_assert_int_eq(int_vector.items[2], 9);
    ck_assert_int_eq(int_vector.items[3], 500);
    ck_assert_int_eq(int_vector.items[4], 15);
    ck_assert_int_eq(int_vector.items[5], 80);
    ck_assert_int_eq(int_vector.items[6], 20);
    
    int_vector_insert(&int_vector, 7, 9001);
    ck_assert_int_eq(int_vector.size, 8);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 50);
    ck_assert_int_eq(int_vector.items[2], 9);
    ck_assert_int_eq(int_vector.items[3], 500);
    ck_assert_int_eq(int_vector.items[4], 15);
    ck_assert_int_eq(int_vector.items[5], 80);
    ck_assert_int_eq(int_vector.items[6], 20);
    ck_assert_int_eq(int_vector.items[7], 9001);

    int_vector_insert(&int_vector, 9, 42);
    ck_assert_int_eq(int_vector.size, 8);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 50);
    ck_assert_int_eq(int_vector.items[2], 9);
    ck_assert_int_eq(int_vector.items[3], 500);
    ck_assert_int_eq(int_vector.items[4], 15);
    ck_assert_int_eq(int_vector.items[5], 80);
    ck_assert_int_eq(int_vector.items[6], 20);
    ck_assert_int_eq(int_vector.items[7], 9001);

    ck_assert_ptr_null(int_vector_ptr);
    int_vector_insert(int_vector_ptr, 0, 50);
    ck_assert_ptr_null(int_vector_ptr);

    int_vector_destructor(&int_vector);
    int_vector_insert(&int_vector, 0, 800);
    ck_assert_int_eq(int_vector.size, 1);
    ck_assert_int_eq(int_vector.items[0], 800);

    int_vector_destructor(&int_vector);
}
END_TEST

START_TEST(delete_test) {
    IntVector int_vector, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);

    int_vector_push(&int_vector, 5);
    int_vector_push(&int_vector, 9);
    int_vector_push(&int_vector, 15);
    int_vector_push(&int_vector, 20);
    ck_assert_int_eq(int_vector.size, 4);
    ck_assert_int_eq(int_vector.items[0], 5);
    ck_assert_int_eq(int_vector.items[1], 9);
    ck_assert_int_eq(int_vector.items[2], 15);
    ck_assert_int_eq(int_vector.items[3], 20);

    int_vector_delete(&int_vector, 0);
    ck_assert_int_eq(int_vector.size, 3);
    ck_assert_int_eq(int_vector.items[0], 9);
    ck_assert_int_eq(int_vector.items[1], 15);
    ck_assert_int_eq(int_vector.items[2], 20);

    int_vector_delete(&int_vector, 1);
    ck_assert_int_eq(int_vector.size, 2);
    ck_assert_int_eq(int_vector.items[0], 9);
    ck_assert_int_eq(int_vector.items[1], 20);

    int_vector_delete(&int_vector, 1);
    ck_assert_int_eq(int_vector.size, 1);
    ck_assert_int_eq(int_vector.items[0], 9);

    int_vector_delete(&int_vector, 0);
    ck_assert_int_vector_empty(&int_vector);

    int_vector_delete(int_vector_ptr, 0);
    ck_assert_ptr_null(int_vector_ptr);
}
END_TEST

START_TEST(get_test) {
    IntVector int_vector, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);

    int_vector_push(&int_vector, 5);
    int_vector_push(&int_vector, 9);
    int_vector_push(&int_vector, 15);
    ck_assert_int_eq(int_vector_get(&int_vector, 0), int_vector.items[0]);
    ck_assert_int_eq(int_vector_get(&int_vector, 1), int_vector.items[1]);
    ck_assert_int_eq(int_vector_get(&int_vector, 2), int_vector.items[2]);
    ck_assert_int_eq(int_vector_get(&int_vector, 3), 3);

    int_vector_push(&int_vector, -20);
    ck_assert_int_eq(int_vector_get(&int_vector, 3), int_vector.items[3]);
    ck_assert_int_eq(int_vector_get(&int_vector, 4), 4);

    int previous_value = int_vector.items[1];
    int_vector_delete(&int_vector, 1);
    ck_assert_int_eq(int_vector_get(&int_vector, 1), int_vector.items[1]);
    ck_assert_int_ne(int_vector_get(&int_vector, 1), previous_value);

    ck_assert_int_eq(int_vector_get(int_vector_ptr, 8), 8);
    ck_assert_ptr_null(int_vector_ptr);

    int_vector_destructor(&int_vector);
}
END_TEST

START_TEST(set_test) {
    IntVector int_vector, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);

    int_vector_push(&int_vector, 5);
    int_vector_push(&int_vector, 9);
    int_vector_push(&int_vector, 15);

    int previous_value = int_vector_get(&int_vector, 2), old_size = int_vector.size;
    ck_assert_int_eq(int_vector_set(&int_vector, 2, -31), 1);
    ck_assert_int_ne(int_vector_get(&int_vector, 2), previous_value);
    ck_assert_int_eq(int_vector.size, old_size);

    ck_assert_int_eq(int_vector_set(&int_vector, 4, 80), 0);
    ck_assert_int_eq(int_vector.size, old_size);

    ck_assert_int_eq(int_vector_set(int_vector_ptr, 2, 55), -1);

    int_vector_destructor(&int_vector);
}
END_TEST

START_TEST(copy_test) {
    IntVector int_vector, int_vec2, *int_vector_ptr = NULL;

    int_vector_constructor(&int_vector);
    int_vector_constructor(&int_vec2);

    int_vector_push(&int_vector, 5);
    int_vector_push(&int_vector, 9);
    int_vector_push(&int_vector, 15);

    int_vector_copy(&int_vec2, &int_vector);
    ck_assert_int_vector_eq(&int_vector, &int_vec2);
    int_vector_destructor(&int_vec2);

    int_vector_copy(int_vector_ptr, &int_vector);
    ck_assert_ptr_null(int_vector_ptr);
    ck_assert_int_vector_notempty(&int_vector);

    int_vector_copy(&int_vector, int_vector_ptr);
    ck_assert_ptr_null(int_vector_ptr);
    ck_assert_int_vector_notempty(&int_vector);

    int_vector_destructor(&int_vector);
}
END_TEST

Suite *int_vector_suite() {
    Suite *suite;
    TCase *create_case, *copy_case, *manipulation_case;
    suite = suite_create("Int Vector");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    manipulation_case = tcase_create("Manipulation");
    tcase_add_test(manipulation_case, resize_test);
    tcase_add_test(manipulation_case, push_test);
    tcase_add_test(manipulation_case, insert_test);
    tcase_add_test(manipulation_case, delete_test);
    tcase_add_test(manipulation_case, get_test);
    tcase_add_test(manipulation_case, set_test);
    suite_add_tcase(suite, manipulation_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    return suite;
}

int main() {
    Suite* suite = int_vector_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
