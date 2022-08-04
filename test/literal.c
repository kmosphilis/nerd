#include <stdlib.h>
#include <check.h>

#include "../src/literal.h"

START_TEST(construct_destruct_test) {
    Literal literal;
    
    literal_construct(&literal, "Penguin", 1);

    ck_assert_str_eq(literal.atom, "Penguin");
    ck_assert_int_eq(literal.sign, 1);

    literal_destruct(&literal);
    ck_assert_ptr_null(literal.atom);
    ck_assert_int_eq(literal.sign, -1);

    literal_construct(&literal, "Penguin", 0);
    ck_assert_str_eq(literal.atom, "Penguin");
    ck_assert_int_eq(literal.sign, 0);

    literal_destruct(&literal);
    ck_assert_ptr_null(literal.atom);
    ck_assert_int_eq(literal.sign, -1);
}
END_TEST

START_TEST(copy_test) {
    Literal literal1, literal2;

    literal_construct(&literal1, "Penguin", 1);

    literal_copy(&literal2, &literal1);

    ck_assert_str_eq(literal1.atom, literal2.atom);
    ck_assert_int_eq(literal1.sign, literal2.sign);
    ck_assert_ptr_ne(&literal1, &literal2);

    literal_destruct(&literal1);
    ck_assert_ptr_null(literal1.atom);
    ck_assert_ptr_nonnull(literal2.atom);

    literal_destruct(&literal2);
    
}
END_TEST

START_TEST(to_string_test) {
    Literal literal;
    literal_construct(&literal, "Penguin", 1);

    char *literal_string = literal_to_string(&literal);
    ck_assert_str_eq(literal_string, "Penguin");
    free(literal_string);

    literal_destruct(&literal);

    literal_construct(&literal, "Penguin", 0);

    literal_string = literal_to_string(&literal);
    ck_assert_str_eq(literal_string, "-Penguin");
    free(literal_string);

    literal_destruct(&literal);
}
END_TEST

START_TEST(negate_test) {
    Literal literal;
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

    literal_destruct(&literal);
}
END_TEST

Suite *literal_suite() {
    Suite *suite;
    TCase *create_case, *copy_case, *string_case, *negate_case;
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

    return suite;
}

int main() {
    Suite* suite = literal_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}