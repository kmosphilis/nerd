#include <stdlib.h>
#include <check.h>

#include "../src/rule.h"
#include "helper/rule.h"
#include "helper/literal.h"

START_TEST(construct_destruct_test) {
    Rule *rule = NULL;
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
    }

    head = literal_constructor("Fly", 0);

    rule = rule_constructor(body_size, body, head, starting_weight);

    ck_assert_int_eq(rule->body->size, body_size);
    ck_assert_literal_eq(rule->head, head);
    for(i = 0; i < body_size; ++i) {
        ck_assert_literal_eq(rule->body->literals[i], body[i]);
    }
    ck_assert_float_eq(rule->weight, starting_weight);

    rule_destructor(&rule);
    ck_assert_ptr_null(rule);

    rule = rule_constructor(0, body, head, starting_weight);
    ck_assert_ptr_null(rule);

    rule = rule_constructor(body_size, NULL, head, starting_weight);
    ck_assert_ptr_null(rule);

    rule = rule_constructor(body_size, body, NULL, starting_weight);
    ck_assert_ptr_null(rule);

    literal_destructor(&head);
    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    free(body);
}
END_TEST

START_TEST(copy_test) {
    Rule *rule1 = NULL, *rule2 = NULL;
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
    }

    head = literal_constructor("Fly", 0);

    rule1 = rule_constructor(body_size, body, head, starting_weight);

    rule_copy(&rule2, rule1);

    ck_assert_ptr_ne(rule1, rule2);
    ck_assert_ptr_ne(rule1->body, rule2->body);
    ck_assert_ptr_ne(rule1->head, rule2->head);
    ck_assert_rule_eq(rule1, rule2);
    rule_destructor(&rule1);
    ck_assert_ptr_null(rule1);
    ck_assert_ptr_nonnull(rule2);

    rule_copy(NULL, rule2);
    ck_assert_ptr_nonnull(rule2);
    rule_destructor(&rule2);

    literal_destructor(&head);
    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    free(body);
}
END_TEST

START_TEST(promote_test) {
    Rule *rule = NULL;
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
    }

    head = literal_constructor("Fly", 0);

    rule = rule_constructor(body_size, body, head, starting_weight);

    rule_promote(rule, 1.89);
    ck_assert_float_eq_tol(rule->weight, starting_weight + 1.89, 0.000001);
    rule_promote(rule, 1.22);
    ck_assert_float_eq_tol(rule->weight, starting_weight + 1.89 + 1.22, 0.000001);
    rule_promote(rule, -1.22);
    ck_assert_float_eq_tol(rule->weight, starting_weight + 1.89, 0.000001);
    rule_promote(rule, -1.89);
    ck_assert_float_eq_tol(rule->weight, starting_weight, 0.000001);
    rule_destructor(&rule);

    ck_assert_ptr_null(rule);
    rule_promote(rule, 1.89);
    ck_assert_ptr_null(rule);


    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }

    free(body);
}
END_TEST

START_TEST(demote_test) {
    Rule *rule = NULL;
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 5;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
    }

    head = literal_constructor("Fly", 0);

    rule = rule_constructor(body_size, body, head, starting_weight);

    rule_demote(rule, 1.89);
    ck_assert_float_eq_tol(rule->weight, starting_weight - 1.89, 0.000001);
    rule_demote(rule, 1.22);
    ck_assert_float_eq_tol(rule->weight, starting_weight - 1.89 - 1.22, 0.000001);
    rule_demote(rule, -1.22);
    ck_assert_float_eq_tol(rule->weight, starting_weight - 1.89, 0.000001);
    rule_demote(rule, -1.89);
    ck_assert_float_eq_tol(rule->weight, starting_weight, 0.000001);
    rule_destructor(&rule);

    ck_assert_ptr_null(rule);
    rule_demote(rule, 1.89);
    ck_assert_ptr_null(rule);


    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }

    free(body);
}
END_TEST

START_TEST(applicable_test) {
    Rule *rule = NULL;
    Context *context = context_constructor();
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
        context_add_literal(context, body[i]);
    }

    head = literal_constructor("Fly", 0);

    rule = rule_constructor(body_size, body, head, starting_weight);
    context_add_literal(context, head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    free(body);
    literal_destructor(&head);

    ck_assert_int_eq(rule_applicable(rule, context), 1);

    scene_remove_literal(context, context->size - 1);
    ck_assert_int_eq(rule_applicable(rule, context),1);

    scene_remove_literal(context, 0);
    ck_assert_int_eq(rule_applicable(rule, context), 0);

    ck_assert_int_eq(rule_applicable(rule, NULL), -1);
    rule_destructor(&rule);
    ck_assert_int_eq(rule_applicable(rule, context), -1);
    ck_assert_ptr_null(rule);
    context_destructor(&context);
}
END_TEST

START_TEST(concurs_test) {
    Rule *rule = NULL;
    Context *context = context_constructor();
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
        context_add_literal(context, body[i]);
    }

    head = literal_constructor("Fly", 0);

    rule = rule_constructor(body_size, body, head, starting_weight);
    context_add_literal(context, head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    free(body);
    literal_destructor(&head);

    ck_assert_int_eq(rule_concurs(rule, context), 1);

    scene_remove_literal(context, 0);
    ck_assert_int_eq(rule_concurs(rule, context), 1);

    scene_remove_literal(context, context->size - 1);
    ck_assert_int_eq(rule_concurs(rule, context), 0);
    ck_assert_int_eq(rule_concurs(rule, NULL), -1);
    rule_destructor(&rule);
    ck_assert_int_eq(rule_concurs(NULL, context), -1);
    ck_assert_ptr_null(rule);
    context_destructor(&context);
}
END_TEST

START_TEST(equality_checK_test) {
    Rule *rule1 = NULL, *rule2 = NULL, *rule3 = NULL, *rule4 = NULL, *rule5 = NULL, *rule6 = NULL,
    *rule7 = NULL;
    const size_t body_size = 3, body_size2 = 2;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    char *body_literal_atoms2[3] = {"Albatross", "Bird", "Antartica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
    }

    head = literal_constructor("Fly", 0);

    rule1 = rule_constructor(body_size, body, head, starting_weight);
    rule_copy(&rule6, rule1);

    rule2 = rule_constructor(body_size, body, head, starting_weight - 0.00001);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
        body[i] = literal_constructor(body_literal_atoms[body_size - i - 1],
        body_literal_signs[body_size - i - 1]);
    }

    rule3 = rule_constructor(body_size, body, head, starting_weight);

    literal_destructor(&head);

    head = literal_constructor("Fly", 1);

    rule4 = rule_constructor(body_size, body, head, starting_weight);
    rule5 = rule_constructor(body_size2, body, head, starting_weight);

    literal_destructor(&head);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
        body[i] = literal_constructor(body_literal_atoms2[i], body_literal_signs[i]);
    }

    rule7 = rule_constructor(body_size, body, rule1->head, starting_weight);

    for (i = 0; i < body_size; ++i) {
        literal_destructor(&body[i]);
    }
    free(body);

    ck_assert_int_eq(rule_equals(rule1, rule6), 1);
    ck_assert_int_eq(rule_equals(rule1, rule2), 1);
    ck_assert_int_eq(rule_equals(rule1, rule3), 1);
    ck_assert_int_ne(rule_equals(rule1, rule4), 1);
    ck_assert_int_eq(rule_equals(rule1, rule4), 0);
    ck_assert_int_ne(rule_equals(rule1, rule5), 1);
    ck_assert_int_eq(rule_equals(rule1, rule5), 0);
    ck_assert_int_ne(rule_equals(rule1, rule7), 1);
    ck_assert_int_eq(rule_equals(rule1, rule7), 0);
    ck_assert_int_ne(rule_equals(rule3, rule4), 1);
    ck_assert_int_eq(rule_equals(rule3, rule4), 0);
    ck_assert_int_ne(rule_equals(rule3, rule5), 1);
    ck_assert_int_eq(rule_equals(rule3, rule5), 0);
    ck_assert_int_ne(rule_equals(rule4, rule5), 1);
    ck_assert_int_eq(rule_equals(rule4, rule5), 0);
    ck_assert_int_eq(rule_equals(rule1, NULL), -1);
    ck_assert_int_eq(rule_equals(NULL, rule1), -1);
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
    Rule *rule = NULL, *copy = NULL;
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
    }

    head = literal_constructor("Fly", 0);

    rule = rule_constructor(body_size, body, head, starting_weight);

    char *rule_string = rule_to_string(rule);
    ck_assert_str_eq(rule_string, "(penguin, bird, antarctica) => -fly (0.0000)");
    free(rule_string);

    rule_string = rule_to_string(NULL);
    ck_assert_pstr_eq(rule_string, NULL);

    Literal *head2 = literal_constructor("Wings", 1);
    literal_destructor(&(rule->head));
    rule_string = rule_to_string(rule);
    ck_assert_pstr_eq(rule_string, NULL);

    literal_copy(&(rule->head), head2);
    literal_destructor(&head2);
    rule->weight = 3.1415;
    rule_string = rule_to_string(rule);
    ck_assert_str_eq(rule_string, "(penguin, bird, antarctica) => wings (3.1415)");
    free(rule_string);

    rule_copy(&copy, rule);
    context_destructor(&(copy->body));
    rule_string = rule_to_string(copy);
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
    Rule *rule = NULL, *copy = NULL;
    const size_t body_size = 3;
    Literal **body = (Literal **) malloc(sizeof(Literal *) * body_size), *head;
    char *body_literal_atoms[3] = {"Penguin", "Bird", "Antarctica"};
    uint_fast8_t body_literal_signs[3] = {1, 1, 1};
    float starting_weight = 0;

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
        body[i] = literal_constructor(body_literal_atoms[i], body_literal_signs[i]);
    }

    head = literal_constructor("Fly", 0);

    rule = rule_constructor(body_size, body, head, starting_weight);

    char *rule_prudensjs_string = rule_to_prudensjs(rule, 1);
    ck_assert_str_eq(rule_prudensjs_string, "{\"name\": \"Rule1\", \"body\": ["
    "{\"name\": \"penguin\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"bird\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}, {\"name\": \"antarctica\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}], \"head\": {\"name\": \"fly\", \"sign\": false, \"isJS\": false, "
    "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, \"arity\": 0}}");
    free(rule_prudensjs_string);

    rule_prudensjs_string = rule_to_prudensjs(copy, 1);

    ck_assert_pstr_eq(rule_prudensjs_string, NULL);

    Literal *head2 = literal_constructor("Wings", 1);
    literal_destructor(&(rule->head));

    rule_prudensjs_string = rule_to_prudensjs(rule, 2);
    ck_assert_pstr_eq(rule_prudensjs_string, NULL);

    literal_copy(&(rule->head), head2);
    literal_destructor(&head2);
    rule->weight = 3.1415;

    rule_prudensjs_string = rule_to_prudensjs(rule, 22);
    ck_assert_str_eq(rule_prudensjs_string, "{\"name\": \"Rule22\", \"body\": ["
    "{\"name\": \"penguin\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"bird\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}, {\"name\": \"antarctica\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}], \"head\": {\"name\": \"wings\", \"sign\": true, \"isJS\": false, "
    "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, \"arity\": 0}}");
    free(rule_prudensjs_string);

    rule_copy(&copy, rule);

    context_destructor(&(copy->body));
    rule_prudensjs_string = rule_to_prudensjs(copy, 2);
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
    tcase_add_test(equality_check_case, concurs_test);
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
