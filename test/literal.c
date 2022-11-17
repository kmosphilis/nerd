#include <stdlib.h>
#include <check.h>

#include "../src/literal.h"
#include "helper/literal.h"

START_TEST(construct_destruct_test) {
    Literal *literal = NULL;

    literal = literal_constructor("Penguin", 1);
    ck_assert_ptr_nonnull(literal);
    ck_assert_str_eq(literal->atom, "penguin");
    ck_assert_int_eq(literal->sign, 1);
    ck_assert_literal_notempty(literal);
    literal_destructor(&literal);
    ck_assert_ptr_null(literal);

    literal = literal_constructor("Penguin", 0);
    ck_assert_str_eq(literal->atom, "penguin");
    ck_assert_int_eq(literal->sign, 0);
    literal_destructor(&literal);
    ck_assert_ptr_null(literal);

    literal = literal_constructor("Penguin", 9);
    ck_assert_ptr_nonnull(literal);
    ck_assert_str_eq(literal->atom, "penguin");
    ck_assert_int_eq(literal->sign, 1);
    literal_destructor(&literal);

    literal = literal_constructor(NULL, 0);
    ck_assert_ptr_nonnull(literal);
    ck_assert_literal_empty(literal);
    literal_destructor(&literal);

    literal_destructor(&literal);
    literal_destructor(NULL);
}
END_TEST

START_TEST(copy_test) {
    Literal *literal1 = NULL, *literal2 = NULL;

    literal1 = literal_constructor("Penguin", 1);

    literal_copy(&literal2, literal1);

    ck_assert_ptr_ne(literal1, literal2);
    ck_assert_literal_eq(literal1, literal2);

    literal_destructor(&literal1);

    ck_assert_ptr_null(literal1);
    ck_assert_ptr_nonnull(literal2);

    literal_destructor(&literal2);

    ck_assert_ptr_null(literal2);

    literal_copy(&literal1, literal2);

    ck_assert_ptr_null(literal1);
    ck_assert_ptr_null(literal2);
}
END_TEST

START_TEST(negate_test) {
    Literal *literal = NULL;
    literal = literal_constructor("Penguin", 1);

    ck_assert_int_eq(literal->sign, 1);
    char *literal_string = literal_to_string(literal);
    ck_assert_str_eq(literal_string, "penguin");
    free(literal_string);

    literal_negate(literal);

    ck_assert_int_eq(literal->sign, 0);
    literal_string = literal_to_string(literal);
    ck_assert_str_eq(literal_string, "-penguin");
    free(literal_string);

    literal_negate(literal);

    ck_assert_int_eq(literal->sign, 1);
    literal_string = literal_to_string(literal);
    ck_assert_str_eq(literal_string, "penguin");
    free(literal_string);
    literal_destructor(&literal);

    literal_negate(literal);

    ck_assert_ptr_null(literal);

}
END_TEST

START_TEST(equality_check_test) {
    Literal *literal1 = NULL, *literal2 = NULL, *literal3 = NULL, *literal4 = NULL;
    
    literal1 = literal_constructor("Penguin", 1);
    literal_copy(&literal2, literal1);
    literal3 = literal_constructor("Penguin", 0);
    literal4 = literal_constructor("Fly", 0);

    ck_assert_int_eq(literal_equals(literal1, literal2), 1);
    ck_assert_int_ne(literal_equals(literal1, literal3), 1);
    ck_assert_int_eq(literal_equals(literal1, literal3), 0);
    ck_assert_int_ne(literal_equals(literal1, literal4), 1);
    ck_assert_int_eq(literal_equals(literal1, literal4), 0);
    ck_assert_int_ne(literal_equals(literal3, literal4), 1);
    ck_assert_int_eq(literal_equals(literal3, literal4), 0);
    ck_assert_int_eq(literal_equals(literal1, NULL), -1);
    ck_assert_int_eq(literal_equals(NULL, literal1), -1);
    ck_assert_int_eq(literal_equals(NULL, NULL), -1);

    literal_destructor(&literal1);
    literal_destructor(&literal2);
    literal_destructor(&literal3);
    literal_destructor(&literal4);
}
END_TEST

START_TEST(opposed_test) {
    Literal *literal1 = NULL, *literal2 = NULL, *literal3 = NULL, *literal4 = NULL;
    
    literal1 = literal_constructor("Penguin", 1);
    literal_copy(&literal2, literal1);
    literal3 = literal_constructor("Penguin", 0);
    literal4 = literal_constructor("Fly", 0);

    ck_assert_int_eq(literal_opposed(literal1, literal2), 0);
    ck_assert_int_ne(literal_opposed(literal1, literal3), 0);
    ck_assert_int_eq(literal_opposed(literal1, literal3), 1);
    ck_assert_int_ne(literal_opposed(literal1, literal4), 1);
    ck_assert_int_eq(literal_opposed(literal1, literal4), -1);
    ck_assert_int_ne(literal_opposed(literal3, literal4), 1);
    ck_assert_int_eq(literal_opposed(literal3, literal4), -1);
    ck_assert_int_eq(literal_opposed(literal1, NULL), -2);
    ck_assert_int_eq(literal_opposed(NULL, literal1), -2);
    ck_assert_int_eq(literal_opposed(NULL, NULL), -2);

    literal_destructor(&literal1);
    literal_destructor(&literal2);
    literal_destructor(&literal3);
    literal_destructor(&literal4);

}
END_TEST

START_TEST(to_string_test) {
    Literal *literal = NULL;
    literal = literal_constructor("Penguin", 1);

    char *literal_string = literal_to_string(literal);
    ck_assert_str_eq(literal_string, "penguin");
    free(literal_string);
    literal_destructor(&literal);

    literal = literal_constructor("Penguin", 0);

    literal_string = literal_to_string(literal);
    ck_assert_str_eq(literal_string, "-penguin");
    free(literal_string);
    literal_destructor(&literal);

    literal_string = literal_to_string(literal);
    ck_assert_pstr_eq(literal_string, NULL);
}
END_TEST

START_TEST(to_prudensjs_test) {
    Literal *literal = NULL;
    literal = literal_constructor("Penguin", 1);

    char *literal_prudensjs_string = literal_to_prudensjs(literal);
    ck_assert_str_eq(literal_prudensjs_string, "{\"name\": \"penguin\", \"sign\": true,"
    " \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}");
    free(literal_prudensjs_string);

    literal_destructor(&literal);

    literal = literal_constructor("Penguin", 0);

    literal_prudensjs_string = literal_to_prudensjs(literal);
    ck_assert_str_eq(literal_prudensjs_string, "{\"name\": \"penguin\", "
    "\"sign\": false, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}");
    free(literal_prudensjs_string);
    literal_destructor(&literal);

    literal_prudensjs_string = literal_to_prudensjs(literal);
    ck_assert_pstr_eq(literal_prudensjs_string, NULL);

    literal = literal_constructor("Albatross", 0);
    literal_prudensjs_string = literal_to_prudensjs(literal);
    ck_assert_str_eq(literal_prudensjs_string, "{\"name\": \"albatross\", "
    "\"sign\": false, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}");
    free(literal_prudensjs_string);
    literal_destructor(&literal);
}
END_TEST

Suite *literal_suite() {
    Suite *suite;
    TCase *create_case, *copy_case, *negate_case, *equality_check_case, *convert_case;
    suite = suite_create("Literal");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    negate_case = tcase_create("Negate");
    tcase_add_test(negate_case, negate_test);
    suite_add_tcase(suite, negate_case);

    equality_check_case = tcase_create("Equality Check");
    tcase_add_test(equality_check_case, equality_check_test);
    tcase_add_test(equality_check_case, opposed_test);
    suite_add_tcase(suite, equality_check_case);

    convert_case = tcase_create("Conversion");
    tcase_add_test(convert_case, to_string_test);
    tcase_add_test(convert_case, to_prudensjs_test);
    suite_add_tcase(suite, convert_case);

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