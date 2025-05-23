#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/nerd.h"
#include "../src/nerd_helper.h"
#include "../src/rule_queue.h"
#include "helper/knowledge_base.h"

#define NUMBER_OF_ACTIVE 4
#define NUMBER_OF_INACTIVE 3
#define NUMBER_OF_ERROR_INPUTS 14
#define NUMBER_OF_TOTAL_LEARNING_ITERATIONS 10

PrudensSettings_ptr settings = NULL;

/**
 * @brief Compares two files.
 *
 * @returns 0 if the files are identical, -1 if they are not, -2 if filepath1
 * does not exist, and -3 if filepath2 does not exist.
 *
 * @param filepath1 Path to the first file.
 * @param filepath2 Path to the second file.
 */
int compare_files(const char *const filepath1, const char *const filepath2) {
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
  RuleQueue *rule_queue1 = create_rule_queue1(),
            *rule_queue2 = create_rule_queue2();

  unsigned int i;

  KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, true);

  for (i = 0; i < rule_queue1->length; ++i) {
    knowledge_base_add_rule(knowledge_base, &(rule_queue1->rules[i]));
    knowledge_base_add_rule(knowledge_base, &(rule_queue2->rules[i]));
  }
  rule_queue_destructor(&rule_queue1);
  rule_queue_destructor(&rule_queue2);

  Nerd *nerd = nerd_constructor(10.0, 5, 3, 100, 0.5, 1.5, true, true);
  ck_assert_ptr_nonnull(nerd);
  ck_assert_int_eq(nerd->max_rules_per_instance, 5);
  ck_assert_int_eq(nerd->breadth, 3);
  ck_assert_int_eq(nerd->depth, 100);
  ck_assert_float_eq_tol(nerd->promotion_weight, 0.5, 0.000001);
  ck_assert_float_eq_tol(nerd->demotion_weight, 1.5, 0.000001);
  ck_assert_int_eq(nerd->increasing_demotion, true);
  ck_assert_float_eq(nerd->promotion_weight, 0.5);
  ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 10.0,
                         0.000001);
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

  nerd = nerd_constructor(10.0, 10, 3, 100, 0.5, 1.5, true, false);
  ck_assert_ptr_nonnull(nerd);
  ck_assert_int_eq(nerd->max_rules_per_instance, 10);
  ck_assert_int_eq(nerd->breadth, 3);
  ck_assert_int_eq(nerd->depth, 100);
  ck_assert_float_eq_tol(nerd->promotion_weight, 0.5, 0.000001);
  ck_assert_float_eq_tol(nerd->demotion_weight, 1.5, 0.000001);
  ck_assert_int_eq(nerd->increasing_demotion, false);
  ck_assert_float_eq(nerd->promotion_weight, 0.5);
  ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 10.0,
                         0.000001);
  ck_assert_knowledge_base_empty(nerd->knowledge_base);
  nerd_destructor(&nerd);
  ck_assert_ptr_null(nerd);

  nerd_destructor(NULL);
}
END_TEST

START_TEST(construct_from_file) {
  Nerd *nerd = nerd_constructor_from_file("../test/data/nerd_input1.txt", true);
  ck_assert_ptr_nonnull(nerd);
  ck_assert_int_eq(nerd->max_rules_per_instance, 5);
  ck_assert_int_eq(nerd->breadth, 2);
  ck_assert_int_eq(nerd->depth, 50);
  ck_assert_float_eq_tol(nerd->promotion_weight, 0.750000, 0.000001);
  ck_assert_float_eq_tol(nerd->demotion_weight, 2.250000, 0.000001);
  ck_assert_int_eq(nerd->increasing_demotion, true);
  ck_assert_float_eq(nerd->promotion_weight, 0.750000);
  ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 6.000000,
                         0.000001);
  ck_assert_knowledge_base_empty(nerd->knowledge_base);
  nerd_destructor(&nerd);

  nerd = nerd_constructor_from_file("../test/data/nerd_input2.txt", true);
  ck_assert_ptr_nonnull(nerd);
  RuleQueue *inactives;
  rule_hypergraph_get_inactive_rules(nerd->knowledge_base, &inactives);
  ck_assert_int_eq(nerd->max_rules_per_instance, 1);
  ck_assert_int_eq(nerd->breadth, 3);
  ck_assert_int_eq(nerd->depth, 100);
  ck_assert_float_eq_tol(nerd->promotion_weight, 0.500000, 0.000001);
  ck_assert_float_eq_tol(nerd->demotion_weight, 1.500000, 0.000001);
  ck_assert_int_eq(nerd->increasing_demotion, 0);
  ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 10.000000,
                         0.000001);
  ck_assert_knowledge_base_notempty(nerd->knowledge_base);
  ck_assert_int_eq(nerd->knowledge_base->active->length, NUMBER_OF_ACTIVE);
  ck_assert_int_eq(inactives->length, NUMBER_OF_INACTIVE);
  char *active_rules[NUMBER_OF_ACTIVE] =
      {"(penguin) => -fly (21.1234)", "(bird, chicken) => -fly (16.0000)",
       "(bird) => fly (15.0055)", "(plane, -bird, -feathers) => fly (10.0000)"},
       *inactive_rules[NUMBER_OF_INACTIVE] = {"(turkey) => -fly (9.0000)",
                                              "(bat, -bird) => fly (9.9999)",
                                              "(feathers) => fly (1.1515)"},
       *str;

  unsigned int i;
  for (i = 0; i < NUMBER_OF_ACTIVE; ++i) {
    str = rule_to_string(nerd->knowledge_base->active->rules[i]);
    ck_assert_str_eq(str, active_rules[i]);
    free(str);
  }

  for (i = 0; i < NUMBER_OF_INACTIVE; ++i) {
    str = rule_to_string(inactives->rules[i]);
    ck_assert_str_eq(str, inactive_rules[i]);
    free(str);
  }
  rule_queue_destructor(&inactives);
  nerd_destructor(&nerd);

  nerd = nerd_constructor_from_file("../test/data/nerd_input2.txt", true);
  ck_assert_ptr_nonnull(nerd);
  rule_hypergraph_get_inactive_rules(nerd->knowledge_base, &inactives);
  ck_assert_int_eq(nerd->breadth, 3);
  ck_assert_int_eq(nerd->depth, 100);
  ck_assert_float_eq_tol(nerd->promotion_weight, 0.500000, 0.000001);
  ck_assert_float_eq_tol(nerd->demotion_weight, 1.500000, 0.000001);
  ck_assert_float_eq_tol(nerd->knowledge_base->activation_threshold, 10.000000,
                         0.000001);
  ck_assert_knowledge_base_notempty(nerd->knowledge_base);
  ck_assert_int_eq(nerd->knowledge_base->active->length, NUMBER_OF_ACTIVE);
  ck_assert_int_eq(inactives->length, NUMBER_OF_INACTIVE);

  for (i = 0; i < NUMBER_OF_ACTIVE; ++i) {
    str = rule_to_string(nerd->knowledge_base->active->rules[i]);
    ck_assert_str_eq(str, active_rules[i]);
    free(str);
  }

  for (i = 0; i < NUMBER_OF_INACTIVE; ++i) {
    str = rule_to_string(inactives->rules[i]);
    ck_assert_str_eq(str, inactive_rules[i]);
    free(str);
  }
  rule_queue_destructor(&inactives);
  nerd_destructor(&nerd);

  for (i = 0; i < NUMBER_OF_ERROR_INPUTS; ++i) {
    char file[BUFFER_SIZE];
    sprintf(file, "../test/data/nerd_input_error%u.txt", i + 1);
    nerd = nerd_constructor_from_file(file, true);
    ck_assert_ptr_null(nerd);
  }

  nerd = nerd_constructor_from_file(NULL, true);
  ck_assert_ptr_null(nerd);

  nerd = nerd_constructor_from_file("../file-that-does-not-exist.txt", true);
  ck_assert_ptr_null(nerd);
}
END_TEST

START_TEST(train_test) {
  Nerd *nerd = nerd_constructor(15.0, 5, 3, 50, 1.5, 4.5, true, true);
  Literal *l1 = literal_constructor_from_string("penuin"),
          *l2 = literal_constructor_from_string("-fly"),
          *l3 = literal_constructor_from_string("bird"), *temp;
  Scene *observation = scene_constructor(true),
        *labels = scene_constructor(true);
  literal_copy(&temp, l2);
  scene_add_literal(observation, &l1);
  scene_add_literal(observation, &l2);
  scene_add_literal(observation, &l3);
  scene_add_literal(labels, &temp);

  unsigned int i;
  for (i = 0; i < NUMBER_OF_TOTAL_LEARNING_ITERATIONS; ++i) {
    nerd_train(nerd, prudensjs_inference, observation, labels, true, NULL, NULL,
               NULL, 0, NULL);
  }

  ck_assert_int_ge(nerd->knowledge_base->active->length, 0);

  for (i = 0; i < nerd->knowledge_base->active->length; ++i) {
    ck_assert_float_gt(nerd->knowledge_base->active->rules[i]->weight, 0);
  }

  RuleQueue *inactive = NULL;
  rule_hypergraph_get_inactive_rules(nerd->knowledge_base, &inactive);
  for (i = 0; i < inactive->length; ++i) {
    ck_assert_float_gt(inactive->rules[i]->weight, 0);
  }

  rule_queue_destructor(&inactive);
  nerd_destructor(&nerd);

  nerd = nerd_constructor(3.0, 5, 3, 50, 1.5, 4.5, true, true);

  for (i = 0; i < NUMBER_OF_TOTAL_LEARNING_ITERATIONS; ++i) {
    nerd_train(nerd, NULL, observation, labels, true, NULL, NULL, NULL, 0,
               NULL);
  }

  ck_assert_int_eq(nerd->knowledge_base->active->length, 0);

  rule_hypergraph_get_inactive_rules(nerd->knowledge_base, &inactive);
  ck_assert_int_gt(inactive->length, 0);
  for (i = 0; i < inactive->length; ++i) {
    ck_assert_float_eq_tol(inactive->rules[i]->weight, 0, 0.000001);
  }

  rule_queue_destructor(&inactive);
  nerd_destructor(&nerd);
  scene_destructor(&observation);
  scene_destructor(&labels);
}
END_TEST

START_TEST(to_file_test) {
  Nerd *nerd = nerd_constructor(15.0, 5, 3, 50, 1.5, 4.5, true, true);

  nerd_to_file(nerd, "../bin/nerd_output1.txt");
  ck_assert_int_eq(compare_files("../bin/nerd_output1.txt",
                                 "../test/data/nerd_expected_output.txt"),
                   0);

  Literal *l1 = literal_constructor_from_string("penuin"),
          *l2 = literal_constructor_from_string("-fly"),
          *l3 = literal_constructor_from_string("bird"), *temp;
  Scene *observation = scene_constructor(true),
        *labels = scene_constructor(true);
  literal_copy(&temp, l2);
  scene_add_literal(observation, &l1);
  scene_add_literal(observation, &l2);
  scene_add_literal(labels, &temp);

  size_t nerd_time_taken = 0, ie_time_taken = 0;

  unsigned int i;
  for (i = 0; i < NUMBER_OF_TOTAL_LEARNING_ITERATIONS; ++i) {
    nerd_train(nerd, prudensjs_inference, observation, labels, false, NULL,
               NULL, NULL, 0, NULL);
  }
  nerd_to_file(nerd, "../bin/nerd_output2.txt");
  ck_assert_int_eq(nerd_time_taken, 0);
  ck_assert_int_eq(ie_time_taken, 0);
  ck_assert_int_eq(compare_files("../bin/nerd_output2.txt",
                                 "../test/data/nerd_expected_output.txt"),
                   -1);

  scene_add_literal(observation, &l3);
  char *previous = knowledge_base_to_string(nerd->knowledge_base);
  for (i = 0; i < NUMBER_OF_TOTAL_LEARNING_ITERATIONS; ++i) {
    nerd_train(nerd, prudensjs_inference, observation, labels, false,
               &nerd_time_taken, &ie_time_taken, NULL, 0, NULL);
  }
  char *current = knowledge_base_to_string(nerd->knowledge_base);
  ck_assert_str_ne(previous, current);
  free(current);
  free(previous);
  ck_assert_int_ge(nerd_time_taken, 0);
  ck_assert_int_gt(ie_time_taken, 0);

  scene_destructor(&labels);
  scene_destructor(&observation);

  nerd_to_file(nerd, NULL);

  nerd_destructor(&nerd);

  nerd_to_file(NULL, "../bin/nerd_output3.txt");
  FILE *file = fopen("../bin/nerd_output3.txt", "rb");
  ck_assert_ptr_null(file);
}
END_TEST

Suite *nerd_suite() {
  Suite *suite = suite_create("Nerd");
  TCase *create_case, *convert_case, *train_case;

  create_case = tcase_create("Create");
  tcase_add_test(create_case, construct_destruct_test);
  tcase_add_test(create_case, construct_from_file);
  suite_add_tcase(suite, create_case);

  train_case = tcase_create("Train");
  tcase_add_test(train_case, train_test);
  suite_add_tcase(suite, train_case);

  convert_case = tcase_create("Convert");
  tcase_add_test(convert_case, to_file_test);
  suite_add_tcase(suite, convert_case);

  return suite;
}

int main(int argc, char *argv[]) {
  prudensjs_settings_constructor(argv[0], NULL, NULL, NULL);
  Suite *suite = nerd_suite();
  SRunner *s_runner;

  s_runner = srunner_create(suite);
  srunner_set_fork_status(s_runner, CK_NOFORK);

  srunner_run_all(s_runner, CK_ENV);
  int number_failed = srunner_ntests_failed(s_runner);
  srunner_free(s_runner);
  prudensjs_settings_destructor();

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
