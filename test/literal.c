#include <stdlib.h>
#include <check.h>

#include "../src/literal.h"
#include "helper/literal.h"

START_TEST(construct_destruct_test) {
    Literal literal, *literal_pointer = NULL;
    
    literal_construct(&literal, "Penguin", 1);

    ck_assert_str_eq(literal.atom, "Penguin");
    ck_assert_int_eq(literal.sign, 1);

    literal_destruct(&literal);
    ck_assert_literal_empty(&literal);

    literal_construct(&literal, "Penguin", 0);
    ck_assert_str_eq(literal.atom, "Penguin");
    ck_assert_int_eq(literal.sign, 0);

    literal_destruct(&literal);
    ck_assert_literal_empty(&literal);

    literal_construct(&literal, "Penguin", 9);

    ck_assert_literal_notempty(&literal);

    ck_assert_str_eq(literal.atom, "Penguin");
    ck_assert_int_eq(literal.sign, 1);

    literal_destruct(&literal);

    literal_construct(&literal, "Penguin", -9);

    ck_assert_str_eq(literal.atom, "Penguin");
    ck_assert_int_eq(literal.sign, 0);

    literal_destruct(&literal);

    literal_construct(literal_pointer, "Penguin", 0);

    ck_assert_ptr_null(literal_pointer);

    literal_destruct(literal_pointer);

    literal_pointer = &literal;

    literal_construct(literal_pointer, NULL, 1);

    ck_assert_literal_empty(literal_pointer);

    literal_destruct(literal_pointer);
}
END_TEST

START_TEST(copy_test) {
    Literal literal1, literal2, *literal_pointer1 = NULL, *literal_pointer2 = NULL;

    literal_construct(&literal1, "Penguin", 1);

    literal_copy(&literal2, &literal1);

    ck_assert_ptr_ne(&literal1, &literal2);
    ck_assert_literal_eq(&literal1, &literal2);

    literal_destruct(&literal1);

    ck_assert_ptr_null(literal1.atom);
    ck_assert_literal_empty(&literal1);
    ck_assert_literal_notempty(&literal2);

    literal_destruct(&literal2);

    ck_assert_literal_empty(&literal2);

    literal_copy(literal_pointer2, literal_pointer1);

    ck_assert_ptr_null(literal_pointer1);
    ck_assert_ptr_null(literal_pointer2);

    literal_pointer1 = &literal1;

    literal_construct(literal_pointer1, "Penguin", 1);

    literal_copy(literal_pointer2, literal_pointer1);

    ck_assert_ptr_null(literal_pointer2);
    ck_assert_literal_notempty(literal_pointer1);

    literal_copy(literal_pointer1, literal_pointer2);

    ck_assert_literal_notempty(literal_pointer1);
    
    literal_destruct(literal_pointer1);
}
END_TEST

START_TEST(to_string_test) {
    Literal literal, *literal_pointer = NULL;
    literal_construct(&literal, "Penguin", 1);

    char *literal_string = literal_to_string(&literal);
    ck_assert_str_eq(literal_string, "Penguin");
    free(literal_string);

    literal_destruct(&literal);

    literal_construct(&literal, "Penguin", 0);

    literal_string = literal_to_string(&literal);
    ck_assert_str_eq(literal_string, "-Penguin");
    free(literal_string);

    literal_string = literal_to_string(literal_pointer);
    ck_assert_pstr_eq(literal_string, NULL);

    literal_pointer = &literal;

    literal_destruct(&literal);

    literal_string = literal_to_string(literal_pointer);
    ck_assert_pstr_eq(literal_string, NULL);
}
END_TEST

START_TEST(negate_test) {
    Literal literal, *literal_pointer = NULL;
    literal_construct(&literal, "Penguin", 1);

    ck_assert_int_eq(literal.sign, 1);
    char *literal_string = literal_to_string(&literal);
    ck_assert_str_eq(literal_string, "Penguin");
    free(literal_string);

    literal_negate(&literal);

    ck_assert_int_eq(literal.sign, 0);
    literal_string = literal_to_string(&literal);
    ck_assert_str_eq(literal_string, "-Penguin");
    free(literal_string);

    literal_negate(&literal);

    ck_assert_int_eq(literal.sign, 1);
    literal_string = literal_to_string(&literal);
    ck_assert_str_eq(literal_string, "Penguin");
    free(literal_string);

    literal_negate(literal_pointer);

    ck_assert_ptr_null(literal_pointer);

    literal_destruct(&literal);
}
END_TEST

START_TEST(equality_check_test) {
    Literal literal1, literal2, literal3, literal4;
    
    literal_construct(&literal1, "Penguin", 1);
    literal_copy(&literal2, &literal1);
    literal_construct(&literal3, "Penguin", 0);
    literal_construct(&literal4, "Fly", 0);

    ck_assert_int_eq(literal_equals(&literal1, &literal2), 1);
    ck_assert_int_ne(literal_equals(&literal1, &literal3), 1);
    ck_assert_int_eq(literal_equals(&literal1, &literal3), 0);
    ck_assert_int_ne(literal_equals(&literal1, &literal4), 1);
    ck_assert_int_eq(literal_equals(&literal1, &literal4), 0);
    ck_assert_int_ne(literal_equals(&literal3, &literal4), 1);
    ck_assert_int_eq(literal_equals(&literal3, &literal4), 0);
    ck_assert_int_eq(literal_equals(&literal1, NULL), -1);
    ck_assert_int_eq(literal_equals(NULL, &literal1), -1);
    ck_assert_int_eq(literal_equals(NULL, NULL), -1);

    literal_destruct(&literal1);
    literal_destruct(&literal2);
    literal_destruct(&literal3);
    literal_destruct(&literal4);
}
END_TEST
Suite *literal_suite() {
    Suite *suite;
    TCase *create_case, *copy_case, *string_case, *negate_case, *equality_check_case;
    suite = suite_create("Literal");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    string_case = tcase_create("To string");
    tcase_add_test(string_case, to_string_test);
    suite_add_tcase(suite, string_case);

    negate_case = tcase_create("Negate");
    tcase_add_test(negate_case, negate_test);
    suite_add_tcase(suite, negate_case);

    equality_check_case = tcase_create("Equality Check");
    tcase_add_test(equality_check_case, equality_check_test);
    suite_add_tcase(suite, equality_check_case);

    return suite;
}

int main() {
    Suite* suite = literal_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}