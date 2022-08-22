#include <stdlib.h>
#include <check.h>

#include "../src/rule.h"
#include "helper/rule.h"
#include "helper/literal.h"

START_TEST(construct_destruct_test) {
    Rule rule, *rule_pointer = NULL;
    const size_t body_size = 3;
    Literal *body = (Literal *) malloc(sizeof(Literal) * body_size);
    Literal head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    int body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        Literal l;
        literal_construct(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_construct(&head, "Fly", 0);

    rule_construct(&rule, body_size, &body, &head, starting_weight);

    ck_assert_rule_notempty(&rule);
    ck_assert_int_eq(rule.body_size, body_size);
    ck_assert_literal_eq(&rule.head, &head);
    for(i = 0; i < body_size; ++i) {
        ck_assert_literal_eq(&rule.body[i], &body[i]);
    }
    ck_assert_float_eq(rule.weight, starting_weight);
    
    rule_destruct(&rule);

    ck_assert_rule_empty(&rule);

    rule_construct(rule_pointer, body_size, &body, &head, starting_weight);

    ck_assert_ptr_null(rule_pointer);

    rule_pointer = &rule;

    rule_construct(rule_pointer, 0, &body, &head, starting_weight);

    ck_assert_rule_empty(rule_pointer);

    rule_destruct(rule_pointer);

    rule_construct(rule_pointer, body_size, NULL, &head, starting_weight);

    ck_assert_rule_empty(rule_pointer);

    rule_destruct(rule_pointer);

    rule_construct(rule_pointer, body_size, &body, NULL, starting_weight);

    ck_assert_rule_empty(rule_pointer);
    
    rule_destruct(rule_pointer);

    literal_destruct(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destruct(&body[i]);
    }
    
    free(body);
}
END_TEST

START_TEST(copy_test) {
    Rule rule1, rule2, *rule_pointer1 = NULL, *rule_pointer2 = NULL;
    const size_t body_size = 3;
    Literal *body = (Literal *) malloc(sizeof(Literal) * body_size);
    Literal head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    int body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        Literal l;
        literal_construct(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_construct(&head, "Fly", 0);

    rule_construct(&rule1, body_size, &body, &head, starting_weight);

    rule_copy(&rule2, &rule1);

    ck_assert_ptr_ne(&rule1, &rule2);
    ck_assert_ptr_ne(rule1.body, rule2.body);
    ck_assert_rule_eq(&rule1, &rule2);
    
    rule_destruct(&rule1);

    ck_assert_rule_empty(&rule1);
    ck_assert_rule_notempty(&rule2);
    ck_assert_literal_ne(&rule1.head, &rule2.head);
    ck_assert_literal_empty(&rule1.head);
    ck_assert_literal_notempty(&rule2.head);

    rule_copy(rule_pointer1, rule_pointer2);

    ck_assert_ptr_null(rule_pointer1);
    ck_assert_ptr_null(rule_pointer2);

    rule_pointer2 = &rule2;

    rule_copy(rule_pointer1, rule_pointer2);

    ck_assert_ptr_null(rule_pointer1);
    ck_assert_rule_notempty(rule_pointer2);

    rule_copy(rule_pointer2, rule_pointer1);

    ck_assert_rule_notempty(rule_pointer2);

    rule_pointer1 = &rule1;

    rule_copy(rule_pointer2, rule_pointer1);

    ck_assert_rule_empty(rule_pointer1);
    ck_assert_rule_notempty(rule_pointer2);

    rule_copy(rule_pointer1, rule_pointer2);

    ck_assert_rule_eq(rule_pointer1, rule_pointer2);

    rule_destruct(rule_pointer1);
    rule_destruct(&rule2);

    literal_destruct(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destruct(&body[i]);
    }
    
    free(body);
}
END_TEST

START_TEST(to_string_test) {
    Rule rule, copy, *rule_pointer = NULL;
    const size_t body_size = 3;
    Literal *body = (Literal *) malloc(sizeof(Literal) * body_size);
    Literal head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    int body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        Literal l;
        literal_construct(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_construct(&head, "Fly", 0);

    rule_construct(&rule, body_size, &body, &head, starting_weight);
    
    char *rule_string = rule_to_string(&rule);

    ck_assert_str_eq(rule_string, "(Penguin, Bird, Antarctica) => -Fly (0.0000)");
    
    free(rule_string);

    rule_string = rule_to_string(rule_pointer);

    ck_assert_pstr_eq(rule_string, NULL);

    Literal head2;
    literal_construct(&head2, "Wings", 1);
    literal_destruct(&rule.head);

    rule_string = rule_to_string(&rule);
    
    ck_assert_pstr_eq(rule_string, NULL);

    literal_copy(&rule.head, &head2);
    literal_destruct(&head2);
    rule.weight = 3.1415;

    rule_string = rule_to_string(&rule);

    ck_assert_str_eq(rule_string, "(Penguin, Bird, Antarctica) => Wings (3.1415)");

    free(rule_string);

    rule_copy(&copy, &rule);

    for (i = 0; i < copy.body_size; ++i) {
        literal_destruct(&(copy.body[i]));
    }
    free(copy.body);
    copy.body = NULL;

    rule_string = rule_to_string(&copy);

    ck_assert_pstr_eq(rule_string, NULL);

    rule_destruct(&copy);
    rule_destruct(&rule);

    literal_destruct(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destruct(&body[i]);
    }
    
    free(body);
}
END_TEST

START_TEST(promote_test) {
    Rule rule, *rule_pointer = NULL;
    const size_t body_size = 3;
    Literal *body = (Literal *) malloc(sizeof(Literal) * body_size);
    Literal head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "North_Pole"};
    int body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        Literal l;
        literal_construct(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_construct(&head, "Fly", 0);

    rule_construct(&rule, body_size, &body, &head, starting_weight);

    rule_promote(&rule, 1.89);
    ck_assert_float_eq_tol(rule.weight, starting_weight + 1.89, 0.000001);
    rule_promote(&rule, 1.22);
    ck_assert_float_eq_tol(rule.weight, starting_weight + 1.89 + 1.22, 0.000001);
    rule_promote(&rule, -1.22);
    ck_assert_float_eq_tol(rule.weight, starting_weight + 1.89, 0.000001);
    rule_promote(&rule, -1.89);
    ck_assert_float_eq_tol(rule.weight, starting_weight, 0.000001);

    rule_promote(rule_pointer, 1.89);

    ck_assert_ptr_null(rule_pointer);

    rule_destruct(&rule);

    literal_destruct(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destruct(&body[i]);
    }
    
    free(body);
}
END_TEST

START_TEST(demote_test) {
    Rule rule, *rule_pointer = NULL;
    const size_t body_size = 3;
    Literal *body = (Literal *) malloc(sizeof(Literal) * body_size);
    Literal head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "North_Pole"};
    int body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 5;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        Literal l;
        literal_construct(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_construct(&head, "Fly", 0);

    rule_construct(&rule, body_size, &body, &head, starting_weight);

    rule_demote(&rule, 1.89);
    ck_assert_float_eq_tol(rule.weight, starting_weight - 1.89, 0.000001);
    rule_demote(&rule, 1.22);
    ck_assert_float_eq_tol(rule.weight, starting_weight - 1.89 - 1.22, 0.000001);
    rule_demote(&rule, -1.22);
    ck_assert_float_eq_tol(rule.weight, starting_weight - 1.89, 0.000001);
    rule_demote(&rule, -1.89);
    ck_assert_float_eq_tol(rule.weight, starting_weight, 0.000001);

    rule_demote(rule_pointer, 1.89);

    ck_assert_ptr_null(rule_pointer);

    rule_destruct(&rule);

    literal_destruct(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destruct(&body[i]);
    }
    
    free(body);
}
END_TEST

Suite *literal_suite() {
    Suite *suite;
    TCase *create_case, *copy_case, *string_case, *weight_adjustment_case;
    suite = suite_create("Rule");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    string_case = tcase_create("To string");
    tcase_add_test(string_case, to_string_test);
    suite_add_tcase(suite, string_case);

    weight_adjustment_case = tcase_create("Weight adjustment");
    tcase_add_test(weight_adjustment_case, promote_test);
    tcase_add_test(weight_adjustment_case, demote_test);
    suite_add_tcase(suite, weight_adjustment_case);

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