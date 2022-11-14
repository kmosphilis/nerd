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
        literal_constructor(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_constructor(&head, "Fly", 0);

    rule_constructor(&rule, body_size, &body, &head, starting_weight);

    ck_assert_rule_notempty(&rule);
    ck_assert_int_eq(rule.body.size, body_size);
    ck_assert_literal_eq(&rule.head, &head);
    for(i = 0; i < body_size; ++i) {
        ck_assert_literal_eq(&rule.body.observations[i], &body[i]);
    }
    ck_assert_float_eq(rule.weight, starting_weight);
    
    rule_destructor(&rule);

    ck_assert_rule_empty(&rule);

    rule_constructor(rule_pointer, body_size, &body, &head, starting_weight);

    ck_assert_ptr_null(rule_pointer);

    rule_pointer = &rule;

    rule_constructor(rule_pointer, 0, &body, &head, starting_weight);

    ck_assert_rule_empty(rule_pointer);

    rule_destructor(rule_pointer);

    rule_constructor(rule_pointer, body_size, NULL, &head, starting_weight);

    ck_assert_rule_empty(rule_pointer);

    rule_destructor(rule_pointer);

    rule_constructor(rule_pointer, body_size, &body, NULL, starting_weight);

    ck_assert_rule_empty(rule_pointer);
    
    rule_destructor(rule_pointer);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
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
        literal_constructor(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_constructor(&head, "Fly", 0);

    rule_constructor(&rule1, body_size, &body, &head, starting_weight);

    rule_copy(&rule2, &rule1);

    ck_assert_ptr_ne(&rule1, &rule2);
    ck_assert_ptr_ne(&(rule1.body), &(rule2.body));
    ck_assert_rule_eq(&rule1, &rule2);
    
    rule_destructor(&rule1);

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

    rule_copy(rule_pointer1, rule_pointer2);

    ck_assert_rule_eq(rule_pointer1, rule_pointer2);

    rule_destructor(rule_pointer1);
    rule_destructor(&rule2);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
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
        literal_constructor(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_constructor(&head, "Fly", 0);

    rule_constructor(&rule, body_size, &body, &head, starting_weight);

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

    rule_destructor(&rule);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
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
        literal_constructor(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_constructor(&head, "Fly", 0);

    rule_constructor(&rule, body_size, &body, &head, starting_weight);

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

    rule_destructor(&rule);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    
    free(body);
}
END_TEST

START_TEST(applicable_test) {
    Rule rule, *rule_pointer = NULL;
    const size_t body_size = 3;
    Literal *body = (Literal *) malloc(sizeof(Literal) * body_size);
    Literal head;
    Context context;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "North_Pole"};
    int body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 5;

    context_constructor(&context);
    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        Literal l;
        literal_constructor(&l, body_literal_atoms[i], body_literal_signs[i]);
        context_add_literal(&context, &l);
        body[i] = l;
    }
    
    literal_constructor(&head, "Fly", 0);
    context_add_literal(&context, &head);

    rule_constructor(&rule, body_size, &body, &head, starting_weight);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    free(body);
    literal_destructor(&head);

    ck_assert_int_eq(rule_applicable(&rule, &context), 1);

    scene_remove_literal(&context, 0);
    ck_assert_int_eq(rule_applicable(&rule, &context), 0);
    
    ck_assert_int_eq(rule_applicable(&rule, NULL), -1);
    ck_assert_int_eq(rule_applicable(rule_pointer, &context), -1);
    ck_assert_ptr_null(rule_pointer);

    context_destructor(&context);
    rule_destructor(&rule);
}
END_TEST

START_TEST(equality_checK_test) {
    Rule rule1, rule2, rule3, rule4, rule5, rule6, rule7;
    const size_t body_size = 3, body_size2 = 2;
    Literal *body = (Literal *) malloc(sizeof(Literal) * body_size);
    Literal head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "North_Pole"};
    char *body_literal_atoms2[3] = {"Albatross", "Bird", "Antartica"};
    int body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 5;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        literal_constructor(&body[i], body_literal_atoms[i], body_literal_signs[i]);
    }

    literal_constructor(&head, "Fly", 0);

    rule_constructor(&rule1, body_size, &body, &head, starting_weight);
    rule_copy(&rule6, &rule1);
    rule_constructor(&rule2, body_size, &body, &head, starting_weight - 0.00001);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
        literal_constructor(&body[i], body_literal_atoms[body_size - i - 1],
        body_literal_signs[body_size - i - 1]);
    }

    rule_constructor(&rule3, body_size, &body, &head, starting_weight);

    literal_destructor(&head);

    literal_constructor(&head, "Fly", 1);

    rule_constructor(&rule4, body_size, &body, &head, starting_weight);
    rule_constructor(&rule5, body_size2, &body, &head, starting_weight);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
        literal_constructor(&body[i], body_literal_atoms2[i], body_literal_signs[i]);
    }

    rule_constructor(&rule7, body_size, &body, &(rule1.head), starting_weight);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }

    free(body);

    ck_assert_int_eq(rule_equals(&rule1, &rule6), 1);
    ck_assert_int_eq(rule_equals(&rule1, &rule2), 1);
    ck_assert_int_eq(rule_equals(&rule1, &rule3), 1);
    ck_assert_int_ne(rule_equals(&rule1, &rule4), 1);
    ck_assert_int_eq(rule_equals(&rule1, &rule4), 0);
    ck_assert_int_ne(rule_equals(&rule1, &rule5), 1);
    ck_assert_int_eq(rule_equals(&rule1, &rule5), 0);
    ck_assert_int_ne(rule_equals(&rule1, &rule7), 1);
    ck_assert_int_eq(rule_equals(&rule1, &rule7), 0);
    ck_assert_int_ne(rule_equals(&rule3, &rule4), 1);
    ck_assert_int_eq(rule_equals(&rule3, &rule4), 0);
    ck_assert_int_ne(rule_equals(&rule3, &rule5), 1);
    ck_assert_int_eq(rule_equals(&rule3, &rule5), 0);
    ck_assert_int_ne(rule_equals(&rule4, &rule5), 1);
    ck_assert_int_eq(rule_equals(&rule4, &rule5), 0);
    ck_assert_int_eq(rule_equals(&rule1, NULL), -1);
    ck_assert_int_eq(rule_equals(NULL, &rule1), -1);
    ck_assert_int_eq(rule_equals(NULL, NULL), -1);

    rule_destructor(&rule1);
    rule_destructor(&rule2);
    rule_destructor(&rule3);
    rule_destructor(&rule4);
    rule_destructor(&rule5);
    rule_destructor(&rule6);
    rule_destructor(&rule7);
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
        literal_constructor(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_constructor(&head, "Fly", 0);

    rule_constructor(&rule, body_size, &body, &head, starting_weight);
    
    char *rule_string = rule_to_string(&rule);

    ck_assert_str_eq(rule_string, "(penguin, bird, antarctica) => -fly (0.0000)");
    
    free(rule_string);

    rule_string = rule_to_string(rule_pointer);

    ck_assert_pstr_eq(rule_string, NULL);

    Literal head2;
    literal_constructor(&head2, "Wings", 1);
    literal_destructor(&rule.head);

    rule_string = rule_to_string(&rule);
    
    ck_assert_pstr_eq(rule_string, NULL);

    literal_copy(&rule.head, &head2);
    literal_destructor(&head2);
    rule.weight = 3.1415;

    rule_string = rule_to_string(&rule);

    ck_assert_str_eq(rule_string, "(penguin, bird, antarctica) => wings (3.1415)");

    free(rule_string);

    rule_copy(&copy, &rule);

    context_destructor(&(copy.body));
    // for (i = 0; i < copy.body.size; ++i) {
    //     literal_destructor(&(copy.body[i]));
    // }
    // free(copy.body);
    // copy.body = NULL;

    rule_string = rule_to_string(&copy);

    ck_assert_pstr_eq(rule_string, NULL);

    rule_destructor(&copy);
    rule_destructor(&rule);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    
    free(body);
}
END_TEST

START_TEST(to_prudensjs_test) {
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
        literal_constructor(&l, body_literal_atoms[i], body_literal_signs[i]);
        body[i] = l;
    }
    
    literal_constructor(&head, "Fly", 0);

    rule_constructor(&rule, body_size, &body, &head, starting_weight);
    
    char *rule_prudensjs_string = rule_to_prudensjs(&rule, 1);

    ck_assert_str_eq(rule_prudensjs_string, "{\"name\": \"Rule1\", \"body\": ["
    "{\"name\": \"penguin\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"bird\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}, {\"name\": \"antarctica\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}], \"head\": {\"name\": \"fly\", \"sign\": false, \"isJS\": false, "
    "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, \"arity\": 0}}");
    
    free(rule_prudensjs_string);

    rule_prudensjs_string = rule_to_prudensjs(rule_pointer, 1);

    ck_assert_pstr_eq(rule_prudensjs_string, NULL);

    Literal head2;
    literal_constructor(&head2, "Wings", 1);
    literal_destructor(&rule.head);

    rule_prudensjs_string = rule_to_prudensjs(&rule, 2);
    
    ck_assert_pstr_eq(rule_prudensjs_string, NULL);

    literal_copy(&rule.head, &head2);
    literal_destructor(&head2);
    rule.weight = 3.1415;

    rule_prudensjs_string = rule_to_prudensjs(&rule, 22);

    ck_assert_str_eq(rule_prudensjs_string, "{\"name\": \"Rule22\", \"body\": ["
    "{\"name\": \"penguin\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"bird\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}, {\"name\": \"antarctica\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}], \"head\": {\"name\": \"wings\", \"sign\": true, \"isJS\": false, "
    "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, \"arity\": 0}}");

    free(rule_prudensjs_string);

    rule_copy(&copy, &rule);

    context_destructor(&(copy.body));
    // for (i = 0; i < copy.body_size; ++i) {
    //     literal_destructor(&(copy.body[i]));
    // }
    // free(copy.body);
    // copy.body = NULL;

    rule_prudensjs_string = rule_to_prudensjs(&copy, 2);

    ck_assert_pstr_eq(rule_prudensjs_string, NULL);

    rule_destructor(&copy);
    rule_destructor(&rule);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    
    free(body);
}
END_TEST

Suite *rule_suite() {
    Suite *suite;
    TCase *create_case, *copy_case, *weight_adjustment_case, *equality_check_case, *convert_case;
    suite = suite_create("Rule");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    weight_adjustment_case = tcase_create("Weight adjustment");
    tcase_add_test(weight_adjustment_case, promote_test);
    tcase_add_test(weight_adjustment_case, demote_test);
    suite_add_tcase(suite, weight_adjustment_case);

    equality_check_case = tcase_create("Equality Check");
    tcase_add_test(equality_check_case, applicable_test);
    tcase_add_test(equality_check_case, equality_checK_test);
    suite_add_tcase(suite, equality_check_case);

    convert_case = tcase_create("Conversion");
    tcase_add_test(convert_case, to_string_test);
    tcase_add_test(convert_case, to_prudensjs_test);
    suite_add_tcase(suite, convert_case);

    return suite;
}

int main() {
    Suite* suite = rule_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}