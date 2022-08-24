#include <stdlib.h>
#include <stdio.h>
#include <check.h>

#include "helper/knowledge_base.h"
#include "helper/rule_queue.h"
#include "helper/rule.h"
#include "../src/knowledge_base.h"

START_TEST(construct_destruct_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;

    knowledge_base_construct(&knowledge_base, 5.0);

    ck_assert_float_eq_tol(knowledge_base.activation_threshold, 5.0, 0.00001);  
    ck_assert_rule_queue_empty(&(knowledge_base.active));
    ck_assert_rule_queue_empty(&(knowledge_base.inactive));

    knowledge_base_construct(knowledge_base_ptr, 5.0);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_destruct(&knowledge_base);
    knowledge_base_destruct(knowledge_base_ptr);

    ck_assert_knowledge_base_empty(&knowledge_base);
}
END_TEST

START_TEST(add_rule_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue, rule_queue1, rule_queue2;

    create_rule_queue(&rule_queue);
    rule_queue_copy(&rule_queue1, &rule_queue);
    rule_queue_construct(&rule_queue2);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue2, &rule);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destruct(&rule);
    }

    knowledge_base_construct(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    rule_queue_destruct(&rule_queue);

    ck_assert_knowledge_base_notempty(&knowledge_base);
    ck_assert_rule_queue_eq(&(knowledge_base.active), &rule_queue2);
    ck_assert_rule_queue_eq(&(knowledge_base.inactive), &rule_queue1);

    knowledge_base_add_rule(knowledge_base_ptr, &rule_queue.rules[0]);
    ck_assert_ptr_null(knowledge_base_ptr);

    rule_queue_destruct(&rule_queue1);
    rule_queue_destruct(&rule_queue2);
    knowledge_base_destruct(&knowledge_base);
}
END_TEST

START_TEST(copy_test) {
    KnowledgeBase knowledge_base1, knowledge_base2, *knowledge_base_ptr1 = NULL,
    *knowledge_base_ptr2 = NULL;
    RuleQueue rule_queue;

    create_rule_queue(&rule_queue);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destruct(&rule);
    }

    knowledge_base_construct(&knowledge_base1, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base1, &(rule_queue.rules[i]));
    }

    rule_queue_destruct(&rule_queue);

    knowledge_base_copy(&knowledge_base2, &knowledge_base1);

    ck_assert_knowledge_base_eq(&knowledge_base1, &knowledge_base2);
    ck_assert_ptr_ne(&(knowledge_base1.active), &(knowledge_base2.active));
    ck_assert_ptr_ne(&(knowledge_base1.inactive), &(knowledge_base2.inactive));

    knowledge_base_destruct(&knowledge_base1);

    ck_assert_knowledge_base_empty(&knowledge_base1);
    ck_assert_knowledge_base_notempty(&knowledge_base2);

    knowledge_base_copy(knowledge_base_ptr2, knowledge_base_ptr1);
    knowledge_base_ptr2 = &knowledge_base2;

    knowledge_base_copy(knowledge_base_ptr2, knowledge_base_ptr1);

    ck_assert_knowledge_base_notempty(knowledge_base_ptr2);

    knowledge_base_copy(knowledge_base_ptr1, knowledge_base_ptr2);

    ck_assert_ptr_null(knowledge_base_ptr1);

    knowledge_base_destruct(&knowledge_base2);
}
END_TEST

START_TEST(applicable_rules_test) {
    KnowledgeBase knowledge_base, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue;
    Literal *literals = NULL;
    size_t literals_size = 4;

    literals = (Literal *) malloc(literals_size * sizeof(Literal));
    char *literal_atoms[4] = {"Penguin", "Bird", "Antarctica", "Fly"};
    int literal_signs[4] = {1, 1, 1, 1};

    unsigned int i;
    for (i = 0; i < literals_size; ++i) {
        literal_construct(&(literals[i]), literal_atoms[i], literal_signs[i]);
    }

    create_rule_queue(&rule_queue);

    knowledge_base_construct(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    rule_queue_destruct(&rule_queue);

    knowledge_base_applicable_rules(&knowledge_base, &literals, literals_size, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 1);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(knowledge_base.inactive.rules[0]));

    rule_queue_destruct(&rule_queue);

    char *literal_atoms2[4] = {"Seagull", "Bird", "Antarctica", "Fly"};

    for (i = 0; i < literals_size; ++i) {
        literal_destruct(&(literals[i]));
        literal_construct(&(literals[i]), literal_atoms2[i], literal_signs[i]);
    }

    knowledge_base_applicable_rules(&knowledge_base, &literals, literals_size, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 0);

    char *literal_atoms3[4] = {"Seagull", "Bird", "Harbor", "Ocean"};

    for (i = 0; i < literals_size; ++i) {
        literal_destruct(&(literals[i]));
        literal_construct(&(literals[i]), literal_atoms3[i], literal_signs[i]);
    }

    knowledge_base_applicable_rules(&knowledge_base, &literals, literals_size, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 1);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(knowledge_base.inactive.rules[2]));

    rule_queue_destruct(&rule_queue);

    char *literal_atoms4[4] = {"Albatross", "Bird", "Antarctica", "Penguin"};

    for (i = 0; i < literals_size; ++i) {
        literal_destruct(&(literals[i]));
        literal_construct(&(literals[i]), literal_atoms4[i], literal_signs[i]);
    }

    knowledge_base_applicable_rules(&knowledge_base, &literals, literals_size, &rule_queue);

    ck_assert_int_eq(rule_queue.length, 2);
    ck_assert_rule_eq(&(rule_queue.rules[0]), &(knowledge_base.inactive.rules[0]));
    ck_assert_rule_eq(&(rule_queue.rules[1]), &(knowledge_base.inactive.rules[1]));
    
    rule_queue_destruct(&rule_queue);

    knowledge_base_applicable_rules(knowledge_base_ptr, &literals, literals_size, &rule_queue);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_applicable_rules(&knowledge_base, NULL, literals_size, &rule_queue);
    ck_assert_rule_queue_empty(&rule_queue);

    knowledge_base_applicable_rules(&knowledge_base, &literals, 0, &rule_queue);
    ck_assert_rule_queue_empty(&rule_queue);

    knowledge_base_applicable_rules(&knowledge_base, &literals, literals_size, NULL);
    ck_assert_rule_queue_empty(&rule_queue);

    for (i = 0; i < literals_size; ++i) {
        literal_destruct(&(literals[i]));
    }

    knowledge_base_destruct(&knowledge_base);
    free(literals);

}
END_TEST

START_TEST(promote_rules_test) {
    KnowledgeBase knowledge_base, knowledge_base2, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue, copy;
    Rule rule;

    unsigned int i;

    create_rule_queue(&rule_queue);

    rule_queue_copy(&copy, &rule_queue);

    knowledge_base_construct(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    ck_assert_rule_queue_empty(&(knowledge_base.active));
    ck_assert_rule_queue_notempty(&(knowledge_base.inactive));

    ck_assert_int_eq(knowledge_base.active.length, 0);
    ck_assert_int_eq(knowledge_base.inactive.length, 3);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 1.0);

    ck_assert_rule_queue_notempty(&(knowledge_base.active));
    ck_assert_int_eq(knowledge_base.active.length, 1);
    ck_assert_int_eq(knowledge_base.inactive.length, 2);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[1]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 1.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 2.0, 0.00001);

    rule_queue_remove_rule(&rule_queue, 2, &rule);
    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 2);
    ck_assert_int_eq(knowledge_base.inactive.length, 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[2]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 5.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 4.0, 0.00001);


    rule_queue_dequeue(&rule_queue, NULL);
    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 6.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 4.0, 0.00001);

    rule_queue_enqueue(&rule_queue, &rule);
    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 3.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 6.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 9.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 4.0, 0.00001);

    knowledge_base_promote_rules(knowledge_base_ptr, &rule_queue, 3.0);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_copy(&knowledge_base2, &knowledge_base);

    knowledge_base_promote_rules(&knowledge_base, NULL, 1.0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, 0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    knowledge_base_promote_rules(&knowledge_base, &rule_queue, -1.0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    rule_destruct(&rule);
    rule_queue_destruct(&rule_queue);
    rule_queue_destruct(&copy);
    knowledge_base_destruct(&knowledge_base);
    knowledge_base_destruct(&knowledge_base2);
}
END_TEST

START_TEST(demote_rules_test) {
    KnowledgeBase knowledge_base, knowledge_base2, *knowledge_base_ptr = NULL;
    RuleQueue rule_queue, copy;
    Rule rule;

    unsigned int i;

    create_rule_queue(&rule_queue);

    rule_queue_copy(&copy, &rule_queue);

    knowledge_base_construct(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        rule_promote(&(rule_queue.rules[i]), 5);
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    ck_assert_rule_queue_notempty(&(knowledge_base.active));
    ck_assert_rule_queue_empty(&(knowledge_base.inactive));

    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 1.0);

    ck_assert_rule_queue_notempty(&(knowledge_base.active));
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[2]), &copy.rules[2]), 1);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 5.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 6.0, 0.00001);

    rule_queue_dequeue(&rule_queue, &rule);
    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 3);
    ck_assert_int_eq(knowledge_base.inactive.length, 0);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[2]), &copy.rules[2]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[2].weight, 5.0, 0.00001);

    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 2);
    ck_assert_int_eq(knowledge_base.inactive.length, 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[1]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.active.rules[1].weight, 3.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);


    rule_queue_dequeue(&rule_queue, NULL);
    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 1.0);
    ck_assert_int_eq(knowledge_base.active.length, 1);
    ck_assert_int_eq(knowledge_base.inactive.length, 2);
    ck_assert_int_eq(rule_equals(&(knowledge_base.active.rules[0]), &copy.rules[0]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[2]), 1);
    ck_assert_float_eq_tol(knowledge_base.active.rules[0].weight, 4.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 2.0, 0.00001);

    rule_queue_enqueue(&rule_queue, &rule);
    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 0);
    ck_assert_int_eq(knowledge_base.inactive.length, 3);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[2]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 0.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[2].weight, 2.0, 0.00001);

    knowledge_base_demote_rules(&knowledge_base, &rule_queue, 2.0);
    ck_assert_int_eq(knowledge_base.active.length, 0);
    ck_assert_int_eq(knowledge_base.inactive.length, 3);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[0]), &copy.rules[1]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[1]), &copy.rules[2]), 1);
    ck_assert_int_eq(rule_equals(&(knowledge_base.inactive.rules[2]), &copy.rules[0]), 1);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[0].weight, 2.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[1].weight, 0.0, 0.00001);
    ck_assert_float_eq_tol(knowledge_base.inactive.rules[3].weight, 0.0, 0.00001);


    knowledge_base_demote_rules(knowledge_base_ptr, &rule_queue, 3.0);
    ck_assert_ptr_null(knowledge_base_ptr);

    knowledge_base_copy(&knowledge_base2, &knowledge_base);

    knowledge_base_demote_rules(&knowledge_base, NULL, 1.0);
    ck_assert_knowledge_base_eq(&knowledge_base, &knowledge_base2);

    rule_destruct(&rule);
    rule_queue_destruct(&rule_queue);
    rule_queue_destruct(&copy);
    knowledge_base_destruct(&knowledge_base);
    knowledge_base_destruct(&knowledge_base2);
}
END_TEST

START_TEST(to_string_test) {
    KnowledgeBase knowledge_base;
    RuleQueue rule_queue;

    create_rule_queue(&rule_queue);

    unsigned int i, rule_queue_length = rule_queue.length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule rule;
        rule_copy(&rule, &(rule_queue.rules[i]));
        rule_promote(&rule, 3.0);
        rule_queue_enqueue(&rule_queue, &rule);
        rule_destruct(&rule);
    }

    knowledge_base_construct(&knowledge_base, 3.0);

    for (i = 0; i < rule_queue.length; ++i) {
        knowledge_base_add_rule(&knowledge_base, &(rule_queue.rules[i]));
    }

    char *knowledge_base_string = knowledge_base_to_string(&knowledge_base);

    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: 3.0000\n"
        "Active Rules: [\n"
        "\t(Penguin, Bird, Antarctica) => -Fly (3.0000),\n"
        "\t(Albatross, Bird) => Fly (4.0000),\n"
        "\t(Seagull, Bird, Harbor, Ocean) => Fly (5.0000)\n]\n"
        "Inactive Rules: [\n"
        "\t(Penguin, Bird, Antarctica) => -Fly (0.0000),\n"
        "\t(Albatross, Bird) => Fly (1.0000),\n"
        "\t(Seagull, Bird, Harbor, Ocean) => Fly (2.0000)\n]");

    free(knowledge_base_string);

    rule_queue_destruct(&rule_queue);

    knowledge_base_destruct(&knowledge_base);
}
END_TEST

Suite *knowledge_base_suite() {
    Suite *suite;
    TCase *create_case, *add_rule_case, *applicable_rule_case, *copy_case, *to_string_case,
    *rule_weight_manipulation_case;
    suite = suite_create("Knowledge Base");

    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    add_rule_case = tcase_create("Add Rule");
    tcase_add_test(add_rule_case, add_rule_test);
    suite_add_tcase(suite, add_rule_case);

    applicable_rule_case = tcase_create("Find Applicable Rules");
    tcase_add_test(applicable_rule_case, applicable_rules_test);
    suite_add_tcase(suite, applicable_rule_case);

    rule_weight_manipulation_case = tcase_create("Rule Weight Manipulation");
    tcase_add_test(rule_weight_manipulation_case, promote_rules_test);
    tcase_add_test(rule_weight_manipulation_case, demote_rules_test);
    suite_add_tcase(suite, rule_weight_manipulation_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    to_string_case = tcase_create("To string");
    tcase_add_test(to_string_case, to_string_test);
    suite_add_tcase(suite, to_string_case);

    return suite;
}

int main() {
    Suite* suite = knowledge_base_suite();
    SRunner* s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}