#include <stdlib.h>
#include <check.h>

#include "../src/rule_queue.h"
#include "helper/rule_queue.h"
#include "helper/rule.h"
#include "helper/int_vector.h"

START_TEST(construct_destruct_test) {
    RuleQueue *rule_queue = rule_queue_constructor();

    ck_assert_rule_queue_empty(rule_queue);

    rule_queue_destructor(&rule_queue);
    ck_assert_ptr_null(rule_queue);

    rule_queue_destructor(&rule_queue);
    ck_assert_ptr_null(rule_queue);

    rule_queue_destructor(NULL);
}
END_TEST

START_TEST(enqueue_test) {
    RuleQueue *rule_queue = rule_queue_constructor();
    Rule *rule = NULL, **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, rules[i]);
    }

    ck_assert_int_eq(rule_queue->length, RULES_TO_CREATE);

    for (i = 0; i < rule_queue->length; ++i) {
        ck_assert_rule_eq(rules[i], rule_queue->rules[i]);
    }

    rule_queue_enqueue(rule_queue, rule);
    ck_assert_int_eq(rule_queue->length, RULES_TO_CREATE);

    rule_queue_enqueue(NULL, rules[0]);
    ck_assert_rule_queue_notempty(rule_queue);

    rule_queue_destructor(&rule_queue);
    ck_assert_ptr_null(rule_queue);

    destruct_rules(rules);
}
END_TEST

START_TEST(dequeue_test) {
    RuleQueue *rule_queue = rule_queue_constructor();

    Rule **rules = create_rules(), *dequeued_rule = NULL;

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, rules[i]);
    }

    for (i = 0; i <rule_queue->length; ++i) {
        ck_assert_rule_eq(rules[i], rule_queue->rules[i]);
    }

    rule_queue_dequeue(rule_queue, &dequeued_rule);
    ck_assert_int_eq(rule_queue->length, 2);
    ck_assert_rule_queue_notempty(rule_queue);
    ck_assert_rule_eq(dequeued_rule, rules[0]);
    rule_destructor(&dequeued_rule);

    for (i = 0; i <rule_queue->length; ++i) {
        ck_assert_rule_eq(rules[i + 1], rule_queue->rules[i]);
    }

    rule_queue_dequeue(rule_queue, NULL);
    ck_assert_rule_queue_notempty(rule_queue);

    for (i = 0; i <rule_queue->length; ++i) {
        ck_assert_rule_eq(rules[i + 2], rule_queue->rules[i]);
    }

    rule_queue_enqueue(rule_queue, rules[1]);
    ck_assert_rule_eq(rules[2], rule_queue->rules[0]);
    ck_assert_rule_eq(rules[1], rule_queue->rules[1]);

    rule_queue_dequeue(rule_queue, &dequeued_rule);
    ck_assert_rule_eq(dequeued_rule, rules[2]);

    rule_destructor(&dequeued_rule);

    ck_assert_rule_eq(rules[1], rule_queue->rules[0]);

    rule_queue_dequeue(rule_queue, NULL);
    ck_assert_rule_queue_empty(rule_queue);

    rule_queue_destructor(&rule_queue);
    ck_assert_ptr_null(rule_queue);
    rule_queue_dequeue(rule_queue, NULL);

    destruct_rules(rules);
}
END_TEST

START_TEST(copy_test) {
    RuleQueue *rule_queue1 = rule_queue_constructor(), *rule_queue2 = NULL;

    Rule **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue1, rules[i]);
    }
    rule_queue_copy(&rule_queue2, rule_queue1);
    ck_assert_ptr_ne(rule_queue1, rule_queue2);
    ck_assert_ptr_ne(rule_queue1->rules, rule_queue2->rules);
    ck_assert_int_eq(rule_queue1->length, rule_queue2->length);

    for (i = 0; i < rule_queue1->length; ++i) {
        ck_assert_rule_eq(rules[i], rule_queue1->rules[i]);
        ck_assert_rule_eq(rules[i], rule_queue2->rules[i]);
        ck_assert_rule_eq(rule_queue1->rules[i], rule_queue2->rules[i]);
        ck_assert_ptr_ne(rule_queue1->rules[i], rule_queue2->rules[i]);
    }
    ck_assert_rule_queue_eq(rule_queue1, rule_queue2);

    rule_queue_destructor(&rule_queue1);
    ck_assert_ptr_null(rule_queue1);
    ck_assert_rule_queue_notempty(rule_queue2);

    for (i = 0; i < rule_queue2->length; ++i) {
        ck_assert_rule_eq(rules[i], rule_queue2->rules[i]);
    }

    rule_queue_copy(NULL, rule_queue2);
    ck_assert_rule_queue_notempty(rule_queue2);

    rule_queue_copy(&rule_queue1, NULL);
    ck_assert_ptr_null(rule_queue1);
    ck_assert_rule_queue_notempty(rule_queue2);
    rule_queue_destructor(&rule_queue2);
    ck_assert_ptr_null(rule_queue2);

    rule_queue_copy(NULL, NULL);

    rule_queue_copy(&rule_queue1, rule_queue2);

    ck_assert_ptr_null(rule_queue2);
    ck_assert_ptr_null(rule_queue1);

    destruct_rules(rules);
}
END_TEST

START_TEST(to_string_test) {
    RuleQueue *rule_queue = rule_queue_constructor();

    Rule **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, rules[i]);
    }

    char *rule_queue_string = rule_queue_to_string(rule_queue);
    ck_assert_str_eq(rule_queue_string, "[\n"
        "\t(penguin, bird, antarctica) => -fly (0.0000),\n"
        "\t(albatross, bird) => fly (1.0000),\n"
        "\t(seagull, bird, harbor, ocean) => fly (2.0000)\n]");
    free(rule_queue_string);

    rule_queue_dequeue(rule_queue, NULL);
    rule_queue_string = rule_queue_to_string(rule_queue);
    ck_assert_str_eq(rule_queue_string, "[\n"
        "\t(albatross, bird) => fly (1.0000),\n"
        "\t(seagull, bird, harbor, ocean) => fly (2.0000)\n]");
    free(rule_queue_string);

    do {
        rule_queue_dequeue(rule_queue, NULL);
    } while (rule_queue->length != 0);

    rule_queue_string = rule_queue_to_string(rule_queue);
    ck_assert_str_eq(rule_queue_string, "[\n]");
    free(rule_queue_string);

    rule_queue_destructor(&rule_queue);
    rule_queue_string = rule_queue_to_string(rule_queue);
    ck_assert_ptr_null(rule_queue);
    ck_assert_pstr_eq(rule_queue_string, NULL);

    destruct_rules(rules);
}
END_TEST

START_TEST(retrieve_index_text) {
    RuleQueue *rule_queue = rule_queue_constructor();

    Rule *rule = NULL, **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, rules[i]);
    }

    ck_assert_int_eq(rule_queue_find(rule_queue, rules[1]), 1);
    ck_assert_int_eq(rule_queue_find(rule_queue, rules[2]), 2);

    rule_queue_dequeue(rule_queue, &rule);
    ck_assert_int_ne(rule_queue_find(rule_queue, rule), 2);
    ck_assert_int_eq(rule_queue_find(rule_queue, rule), -1);

    rule_queue_dequeue(rule_queue, NULL);
    rule_queue_enqueue(rule_queue, rule);
    ck_assert_int_eq(rule_queue_find(rule_queue, rule), 1);
    ck_assert_int_eq(rule_queue_find(NULL, rule), -2);

    rule_destructor(&rule);
    ck_assert_int_eq(rule_queue_find(rule_queue, rule), -2);

    rule_queue_destructor(&rule_queue);

    destruct_rules(rules);
}
END_TEST

START_TEST(remove_indexed_rule_test) {
    RuleQueue *rule_queue = rule_queue_constructor();

    Rule *rule = NULL, **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, rules[i]);
    }

    int rule_index = rule_queue_find(rule_queue, rules[1]);
    ck_assert_int_eq(rule_index, 1);

    rule_queue_remove_rule(rule_queue, rule_index, NULL);
    ck_assert_rule_eq(rule_queue->rules[0], rules[0]);
    ck_assert_rule_eq(rule_queue->rules[1], rules[2]);

    rule_queue_enqueue(rule_queue, rules[0]);
    rule_queue_remove_rule(rule_queue, 0, NULL);
    ck_assert_rule_eq(rule_queue->rules[0], rules[2]);
    ck_assert_rule_eq(rule_queue->rules[1], rules[0]);

    rule_queue_enqueue(rule_queue, rules[1]);
    rule_queue_remove_rule(rule_queue, 2, &rule);
    ck_assert_rule_eq(rule_queue->rules[0], rules[2]);
    ck_assert_rule_eq(rule_queue->rules[1], rules[0]);
    ck_assert_rule_eq(rule, rules[1]);
    rule_destructor(&rule);

    rule_queue_remove_rule(rule_queue, 8, NULL);
    ck_assert_rule_eq(rule_queue->rules[0], rules[2]);
    ck_assert_rule_eq(rule_queue->rules[1], rules[0]);

    rule_queue_remove_rule(rule_queue, -1, NULL);
    ck_assert_rule_eq(rule_queue->rules[0], rules[2]);
    ck_assert_rule_eq(rule_queue->rules[1], rules[0]);

    rule_queue_destructor(&rule_queue);
    ck_assert_ptr_null(rule_queue);
    rule_queue_remove_rule(rule_queue, 9, NULL);
    ck_assert_ptr_null(rule_queue);

    destruct_rules(rules);
}
END_TEST

START_TEST(find_applicable_rules_test) {
    RuleQueue *rule_queue = rule_queue_constructor();
    Context *context = context_constructor();
    IntVector *result = NULL;

    Rule **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, rules[i]);
    }

    Literal *literal = literal_constructor("Penguin", 1);
    context_add_literal(context, literal);
    literal_destructor(&literal);

    literal = literal_constructor("Antarctica", 1);
    context_add_literal(context, literal);
    literal_destructor(&literal);

    literal = literal_constructor("Bird", 1);
    context_add_literal(context, literal);
    literal_destructor(&literal);

    rule_queue_find_applicable_rules(rule_queue, context, &result);
    ck_assert_int_vector_notempty(result);
    ck_assert_int_eq(result->size, 1);
    ck_assert_int_eq(int_vector_get(result, 0), 0);
    int_vector_destructor(&result);

    rule_queue_find_applicable_rules(NULL, context, &result);
    ck_assert_ptr_null(result);

    rule_queue_find_applicable_rules(rule_queue, NULL, &result);
    ck_assert_ptr_null(result);

    rule_queue_find_applicable_rules(rule_queue, context, NULL);

    context_destructor(&context);
    rule_queue_destructor(&rule_queue);
    destruct_rules(rules);
}

START_TEST(find_concurring_rules_test) {
    RuleQueue *rule_queue = rule_queue_constructor();
    Context *context = context_constructor();
    IntVector *result = NULL;

    Rule **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, rules[i]);
    }

    Literal *literal = literal_constructor("Penguin", 1);
    context_add_literal(context, literal);
    literal_destructor(&literal);

    literal = literal_constructor("Antarctica", 1);
    context_add_literal(context, literal);
    literal_destructor(&literal);

    literal = literal_constructor("Bird", 1);
    context_add_literal(context, literal);
    literal_destructor(&literal);

    rule_queue_find_concurring_rules(rule_queue, context, &result);
    ck_assert_int_vector_empty(result);
    int_vector_destructor(&result);

    literal = literal_constructor("Fly", 0);
    context_add_literal(context, literal);
    literal_destructor(&literal);

    rule_queue_find_concurring_rules(rule_queue, context, &result);
    ck_assert_int_vector_notempty(result);
    ck_assert_int_eq(result->size, 1);
    ck_assert_int_eq(int_vector_get(result, 0), 0);
    int_vector_destructor(&result);

    rule_queue_find_concurring_rules(NULL, context, &result);
    ck_assert_ptr_null(result);

    rule_queue_find_concurring_rules(rule_queue, NULL, &result);
    ck_assert_ptr_null(result);

    rule_queue_find_concurring_rules(rule_queue, context, NULL);

    context_destructor(&context);
    rule_queue_destructor(&rule_queue);
    destruct_rules(rules);
}

Suite *rule_queue_suite() {
    Suite *suite;
    TCase *create_case, *operations_case, *copy_case, *string_case;
    suite = suite_create("Rule Queue");

    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    operations_case = tcase_create("Operations");
    tcase_add_test(operations_case, enqueue_test);
    tcase_add_test(operations_case, dequeue_test);
    tcase_add_test(operations_case, retrieve_index_text);
    tcase_add_test(operations_case, remove_indexed_rule_test);
    tcase_add_test(operations_case, find_applicable_rules_test);
    tcase_add_test(operations_case, find_concurring_rules_test);
    suite_add_tcase(suite, operations_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    string_case = tcase_create("To string");
    tcase_add_test(string_case, to_string_test);
    suite_add_tcase(suite, string_case);

    return suite;
}

int main() {
    Suite* suite = rule_queue_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
