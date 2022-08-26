#include <stdlib.h>
#include <check.h>

#include "../src/rule_queue.h"
#include "helper/rule_queue.h"
#include "helper/rule.h"

START_TEST(construct_destruct_test) {
    RuleQueue rule_queue, *rule_queue_pointer = NULL;

    rule_queue_constructor(&rule_queue);

    ck_assert_rule_queue_empty(&rule_queue);

    rule_queue_destructor(&rule_queue);

    ck_assert_rule_queue_empty(&rule_queue);

    rule_queue_constructor(rule_queue_pointer);

    ck_assert_ptr_null(rule_queue_pointer);
}
END_TEST

START_TEST(enqueue_test) {
    RuleQueue rule_queue, *rule_queue_pointer = NULL;
    
    rule_queue_constructor(&rule_queue);

    Rule *rule_pointer = NULL, *rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(&rule_queue, &(rules[i]));
    }

    ck_assert_int_eq(rule_queue.length, RULES_TO_CREATE);

    rule_queue_enqueue(rule_queue_pointer, &(rules[0]));

    rule_queue_enqueue(&rule_queue, rule_pointer);

    ck_assert_int_eq(rule_queue.length, RULES_TO_CREATE);

    ck_assert_ptr_null(rule_queue_pointer);

    ck_assert_rule_queue_notempty(&rule_queue);

    for (i = 0; i <rule_queue.length; ++i) {
        ck_assert_rule_eq(&(rules[i]), &(rule_queue.rules[i]));
    }

    rule_queue_destructor(&rule_queue);

    destruct_rules(rules);
}
END_TEST

START_TEST(dequeue_test) {
    RuleQueue rule_queue, *rule_queue_pointer = NULL;
    
    rule_queue_constructor(&rule_queue);

    Rule *rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(&rule_queue, &(rules[i]));
    }

    for (i = 0; i <rule_queue.length; ++i) {
        ck_assert_rule_eq(&(rules[i]), &(rule_queue.rules[i]));
    }

    Rule dequeued_rule, *rule_pointer = NULL;

    rule_queue_dequeue(&rule_queue, &dequeued_rule);
    
    ck_assert_int_eq(rule_queue.length, 2);

    ck_assert_rule_queue_notempty(&rule_queue);
    
    ck_assert_rule_eq(&dequeued_rule, &(rules[0]));

    rule_destructor(&dequeued_rule);

    for (i = 0; i <rule_queue.length; ++i) {
        ck_assert_rule_eq(&(rules[i + 1]), &(rule_queue.rules[i]));
    }

    rule_queue_dequeue(&rule_queue, NULL);

    rule_queue_dequeue(rule_queue_pointer, NULL);

    ck_assert_ptr_null(rule_queue_pointer);

    ck_assert_rule_queue_notempty(&rule_queue);

    for (i = 0; i <rule_queue.length; ++i) {
        ck_assert_rule_eq(&(rules[i + 2]), &(rule_queue.rules[i]));
    }

    rule_queue_enqueue(&rule_queue, &(rules[1]));

    ck_assert_rule_eq(&(rules[2]), &(rule_queue.rules[0]));
    ck_assert_rule_eq(&(rules[1]), &(rule_queue.rules[1]));

    rule_queue_dequeue(&rule_queue, &dequeued_rule);

    ck_assert_rule_eq(&dequeued_rule, &(rules[2]));

    rule_destructor(&dequeued_rule);

    ck_assert_rule_eq(&(rules[1]), &(rule_queue.rules[0]));

    rule_queue_dequeue(&rule_queue, rule_pointer);

    ck_assert_ptr_null(rule_pointer);

    ck_assert_rule_queue_empty(&rule_queue);

    rule_queue_destructor(&rule_queue);

    destruct_rules(rules);
}
END_TEST

START_TEST(copy_test) {
    RuleQueue rule_queue1, rule_queue2, *rule_queue_pointer1 = NULL, *rule_queue_pointer2 = NULL;

    rule_queue_constructor(&rule_queue1);

    Rule *rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(&rule_queue1, &(rules[i]));
    }

    rule_queue_copy(&rule_queue2, &rule_queue1);

    ck_assert_ptr_ne(&rule_queue1, &rule_queue2);

    for (i = 0; i < rule_queue1.length; ++i) {
        ck_assert_rule_eq(&(rules[i]), &(rule_queue1.rules[i]));
        ck_assert_rule_eq(&(rules[i]), &(rule_queue2.rules[i]));
        ck_assert_rule_eq(&(rule_queue1.rules[i]), &(rule_queue2.rules[i]));
        ck_assert_ptr_ne(&(rule_queue1.rules[i]), &(rule_queue2.rules[i]));
    }

    rule_queue_destructor(&rule_queue1);

    for (i = 0; i < rule_queue1.length; ++i) {
        ck_assert_rule_eq(&(rules[i]), &(rule_queue2.rules[i]));
    }

    rule_queue_copy(rule_queue_pointer1, rule_queue_pointer2);

    ck_assert_ptr_null(rule_queue_pointer1);
    ck_assert_ptr_null(rule_queue_pointer2);

    rule_queue_pointer2 = &rule_queue2;
    
    ck_assert_rule_queue_notempty(rule_queue_pointer2);
    
    rule_queue_copy(rule_queue_pointer2, rule_queue_pointer1);

    ck_assert_ptr_null(rule_queue_pointer1);
    ck_assert_rule_queue_notempty(rule_queue_pointer2);

    rule_queue_copy(rule_queue_pointer1, rule_queue_pointer2);

    ck_assert_ptr_null(rule_queue_pointer1);
    ck_assert_rule_queue_notempty(rule_queue_pointer2);

    rule_queue_pointer1 = &rule_queue1;

    rule_queue_copy(rule_queue_pointer1, rule_queue_pointer2);

    ck_assert_rule_queue_eq(rule_queue_pointer1, rule_queue_pointer2);

    rule_queue_destructor(rule_queue_pointer1);
    rule_queue_destructor(&rule_queue2);

    destruct_rules(rules);
}
END_TEST

START_TEST(to_string_test) {
    RuleQueue rule_queue, *rule_queue_pointer = NULL;
    
    rule_queue_constructor(&rule_queue);

    Rule *rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(&rule_queue, &(rules[i]));
    }

    char *rule_queue_string = rule_queue_to_string(&rule_queue);

    ck_assert_str_eq(rule_queue_string, "RuleQueue: [\n"
        "\t(Penguin, Bird, Antarctica) => -Fly (0.0000),\n"
        "\t(Albatross, Bird) => Fly (1.0000),\n"
        "\t(Seagull, Bird, Harbor, Ocean) => Fly (2.0000)\n]");

    free(rule_queue_string);

    rule_queue_dequeue(&rule_queue, NULL);

    rule_queue_string = rule_queue_to_string(&rule_queue);

    ck_assert_str_eq(rule_queue_string, "RuleQueue: [\n"
        "\t(Albatross, Bird) => Fly (1.0000),\n"
        "\t(Seagull, Bird, Harbor, Ocean) => Fly (2.0000)\n]");

    free(rule_queue_string);

    rule_queue_string = rule_queue_to_string(rule_queue_pointer);

    ck_assert_pstr_eq(rule_queue_string, NULL);
    
    do {
        rule_queue_dequeue(&rule_queue, NULL);
    } while (rule_queue.length != 0);

    rule_queue_string = rule_queue_to_string(&rule_queue);

    ck_assert_str_eq(rule_queue_string, "RuleQueue: [\n]");

    free(rule_queue_string);

    rule_queue_destructor(&rule_queue);

    rule_queue_string = rule_queue_to_string(&rule_queue);

    ck_assert_str_eq(rule_queue_string, "RuleQueue: [\n]");

    free(rule_queue_string);

    destruct_rules(rules);
}
END_TEST

START_TEST(remove_indexed_rule_test) {
    RuleQueue rule_queue, *rule_queue_pointer = NULL;
    
    rule_queue_constructor(&rule_queue);

    Rule rule, *rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(&rule_queue, &(rules[i]));
    }

    int rule_index = rule_queue_find(&rule_queue, &(rules[1]));

    ck_assert_int_eq(rule_index, 1);

    rule_queue_remove_rule(&rule_queue, rule_index, NULL);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(rules[0]));
    ck_assert_rule_eq(&(rule_queue.rules[1]), &(rules[2]));

    rule_queue_enqueue(&rule_queue, &(rules[0]));
    rule_queue_remove_rule(&rule_queue, 0, NULL);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(rules[2]));
    ck_assert_rule_eq(&(rule_queue.rules[1]), &(rules[0]));

    rule_queue_enqueue(&rule_queue, &(rules[1]));
    rule_queue_remove_rule(&rule_queue, 2, &rule);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(rules[2]));
    ck_assert_rule_eq(&(rule_queue.rules[1]), &(rules[0]));
    ck_assert_rule_eq(&rule, &(rules[1]));
    rule_destructor(&rule);

    rule_queue_remove_rule(&rule_queue, 8, NULL);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(rules[2]));
    ck_assert_rule_eq(&(rule_queue.rules[1]), &(rules[0]));

    rule_queue_remove_rule(&rule_queue, -1, NULL);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(rules[2]));
    ck_assert_rule_eq(&(rule_queue.rules[1]), &(rules[0]));

    rule_queue_remove_rule(rule_queue_pointer, 9, NULL);
    ck_assert_ptr_null(rule_queue_pointer);

    rule_queue_destructor(&rule_queue);

    destruct_rules(rules);
}
END_TEST

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
    tcase_add_test(operations_case, remove_indexed_rule_test);
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