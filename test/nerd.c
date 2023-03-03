#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "../src/nerd.h"
#include "helper/knowledge_base.h"

#define DATASET "../data/test.txt"
#define NUMBER_OF_ACTIVE 4
#define NUMBER_OF_INACTIVE 3

/**
 * @brief Compares two files.
 *
 * @returns 0 if the files are identical, -1 if they are not, -2 if filepath1 does not exist, and
 * -3 if filepath2 does not exist.
 *
 * @param filepath1 Path to the first file.
 * @param filepath2 Path to the second file.
*/
int compare_files(const char * const filepath1, const char * const filepath2) {
    FILE *file1 = fopen(filepath1, "rb");
    FILE *file2 = fopen(filepath2, "rb");

    if (!file1) {
        if (!file2) {
            return -3;
        }
        fclose(file2);
        return -1;
    } else if (!file2) {
        fclose(file1);
        return -2;
    }

    char file1_char = fgetc(file1), file2_char = fgetc(file2);

    while ((file1_char != EOF) && (file2_char != EOF)) {
        if (file1_char != file2_char) {
            break;
        }
        file1_char = fgetc(file1);
        file2_char = fgetc(file2);
    }

    fclose(file1);
    fclose(file2);

    if ((file1_char == EOF) && (file2_char == EOF)) {
        return 0;
    }

    return -1;
}

START_TEST(construct_destruct_test) {
    RuleQueue *rule_queue = create_rule_queue();

    unsigned int i, rule_queue_length = rule_queue->length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule *rule = NULL;
        rule_copy(&rule, rule_queue->rules[i]);
        rule_promote(rule, 3.0);
        rule_queue_enqueue(rule_queue, &rule);
    }

    KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0);

    for (i = 0; i < rule_queue->length; ++i) {
        knowledge_base_add_rule(knowledge_base, &rule_queue->rules[i]);
    }

    rule_queue_destructor(&rule_queue);

    Nerd *nerd = nerd_constructor(DATASET, 1, 10.0, 3, 100, 50, 0.5, 1.5, 0);
    ck_assert_int_eq(nerd->breadth, 3);
    ck_assert_int_eq(nerd->depth, 100);
    ck_assert_int_eq(nerd->epochs, 50);
    ck_assert_float_eq_tol(nerd->promotion_weight, 0.5, 0.000001);
    ck_assert_float_eq_tol(nerd->demotion_weight, 1.5, 0.000001);
    ck_assert_float_eq(nerd->promotion_weight, 0.5);
    ck_assert_int_eq(nerd->partial_observation, 0);
    ck_assert_ptr_nonnull(nerd->sensor->environment);
    ck_assert_int_eq(nerd->sensor->reuse, 1);
    ck_assert_str_eq(nerd->sensor->filepath, DATASET);
    ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 10.0, 0.000001);
    ck_assert_knowledge_base_empty(nerd->knowledge_base);

    knowledge_base_destructor(&(nerd->knowledge_base));
    knowledge_base_copy(&(nerd->knowledge_base), knowledge_base);
    ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold,
    knowledge_base->activation_threshold, 0.000001);
    ck_assert_knowledge_base_notempty(nerd->knowledge_base);
    ck_assert_knowledge_base_eq(nerd->knowledge_base, knowledge_base);

    knowledge_base_destructor(&knowledge_base);

    nerd_destructor(&nerd);
    ck_assert_ptr_null(nerd);

    nerd = nerd_constructor(NULL, 1, 10.0, 3, 100, 50, 0.5, 1.5, 0);
    ck_assert_ptr_null(nerd);

    nerd_destructor(&nerd);
    ck_assert_ptr_null(nerd);

    nerd_destructor(NULL);
}
END_TEST

START_TEST(construct_from_file) {
    Nerd *nerd = nerd_constructor_from_file("../test/data/nerd_input1.txt", 2);
    ck_assert_int_eq(nerd->breadth, 2);
    ck_assert_int_eq(nerd->depth, 50);
    ck_assert_int_eq(nerd->epochs, 2);
    ck_assert_float_eq_tol(nerd->promotion_weight, 0.750000, 0.000001);
    ck_assert_float_eq_tol(nerd->demotion_weight, 2.250000, 0.000001);
    ck_assert_float_eq(nerd->promotion_weight, 0.750000);
    ck_assert_int_eq(nerd->partial_observation, 1);
    ck_assert_ptr_nonnull(nerd->sensor->environment);
    ck_assert_int_eq(nerd->sensor->reuse, 0);
    ck_assert_str_eq(nerd->sensor->filepath, "../data/test.txt");
    ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 6.000000, 0.000001);
    ck_assert_knowledge_base_empty(nerd->knowledge_base);
    nerd_destructor(&nerd);

    nerd = nerd_constructor_from_file("../test/data/nerd_input2.txt", 1);
    ck_assert_int_eq(nerd->breadth, 3);
    ck_assert_int_eq(nerd->depth, 100);
    ck_assert_int_eq(nerd->epochs, 1);
    ck_assert_float_eq_tol(nerd->promotion_weight, 0.500000, 0.000001);
    ck_assert_float_eq_tol(nerd->demotion_weight, 1.500000, 0.000001);
    ck_assert_int_eq(nerd->partial_observation, 0);
    ck_assert_ptr_nonnull(nerd->sensor->environment);
    ck_assert_int_eq(nerd->sensor->reuse, 1);
    ck_assert_str_eq(nerd->sensor->filepath, "../data/test.txt");
    ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 10.000000, 0.000001);
    ck_assert_knowledge_base_notempty(nerd->knowledge_base);
    ck_assert_int_eq(nerd->knowledge_base->active->length, NUMBER_OF_ACTIVE);
    ck_assert_int_eq(nerd->knowledge_base->inactive->length, NUMBER_OF_INACTIVE);
    char *active_rules[NUMBER_OF_ACTIVE] = {"(penguin) => -fly (21.1234)",
    "(bird, chicken) => -fly (16.0000)", "(bird) => fly (15.0055)",
    "(plane, -bird, -feathers) => fly (10.0000)"},
    *inactive_rules[NUMBER_OF_INACTIVE] = {"(bat, -bird) => fly (9.9999)",
    "(turkey) => -fly (9.0000)", "(feathers) => fly (1.1515)"}, *str;

    unsigned int i;
    for (i = 0; i < NUMBER_OF_ACTIVE; ++i) {
        str = rule_to_string(nerd->knowledge_base->active->rules[i]);
        ck_assert_str_eq(str, active_rules[i]);
        free(str);
    }

    for (i = 0; i < NUMBER_OF_INACTIVE; ++i) {
        str = rule_to_string(nerd->knowledge_base->inactive->rules[i]);
        ck_assert_str_eq(str, inactive_rules[i]);
        free(str);
    }
    nerd_destructor(&nerd);

    nerd = nerd_constructor_from_file(NULL, 1);
    ck_assert_ptr_null(nerd);

    nerd = nerd_constructor_from_file("../file-that-doesnt_exist.txt", 1);
    ck_assert_ptr_null(nerd);
}
END_TEST

START_TEST(to_file_test) {
    Nerd *nerd = nerd_constructor(DATASET, 1, 15.0, 3, 50, 1, 1.5, 4.5, 1);

    nerd_to_file(nerd, "../bin/nerd_output1.txt");
    ck_assert_int_eq(compare_files("../bin/nerd_output1.txt",
    "../test/data/nerd_expected_output.txt"), 0);

    nerd_start_learning(nerd);
    nerd_to_file(nerd, "../bin/nerd_output2.txt");
    ck_assert_int_eq(compare_files("../bin/nerd_output2.txt",
    "../test/data/nerd_expected_output.txt"), -1);

    nerd_to_file(nerd, NULL);

    nerd_destructor(&nerd);

    nerd_to_file(NULL, "../bin/nerd_output3.txt");
    FILE *file = fopen("../bin/nerd_output3.txt", "rb");
    ck_assert_ptr_null(file);
}
END_TEST

Suite *nerd_suite() {
    Suite *suite;
    TCase *create_case, *convert_case;

    suite = suite_create("Nerd");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    tcase_add_test(create_case, construct_from_file);
    suite_add_tcase(suite, create_case);

    convert_case = tcase_create("Convert");
    tcase_add_test(convert_case, to_file_test);
    suite_add_tcase(suite, convert_case);

    return suite;
}

int main() {
    Suite *suite = nerd_suite();
    SRunner *s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
