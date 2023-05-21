#include <check.h>

#include "../src/nerd.h"
#include "../src/knowledge_base.h"
#include "../src/context.h"
#include "../src/metrics.h"

#define DATASET1 "../test/data/sensor_test1.txt"
#define DATASET2 "../test/data/sensor_test3.txt"

START_TEST(all_literals_evaluation_test) {
    Nerd *nerd = nerd_constructor(DATASET1, ' ', true, false, 15.0, 3, 50, 1, 1.5, 4.5, 1);
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true);
    Rule *r1 = rule_constructor(1, &l1, &l2, 15.0, false),
    *r2 = rule_constructor(1, &l3, &l1, 16.0, false);
    Context *observation;
    sensor_get_next_scene(nerd->sensor, &observation, false, NULL);

    float overall_success, old_overall;
    evaluate_all_literals(nerd, observation, &overall_success);
    ck_assert_double_eq(overall_success, 0);

    knowledge_base_add_rule(nerd->knowledge_base, &r1);
    knowledge_base_add_rule(nerd->knowledge_base, &r2);
    old_overall = overall_success;
    evaluate_all_literals(nerd, observation, &overall_success);
    ck_assert_double_ne(overall_success, old_overall);

    context_destructor(&observation);
    nerd_destructor(&nerd);
}
END_TEST

START_TEST(random_literals_evaluation_test) {
    Nerd *nerd = nerd_constructor(DATASET2, ',', true, true, 15.0, 3, 50, 1, 1.5, 4.5, 1);
    Literal *l1 = literal_constructor("class_bird", true),
    *l2 = literal_constructor("flies?_yes", true), *l3 = literal_constructor("class_mammal", true),
    *l4 = literal_constructor("flies?_no", true);
    Rule *r1 = rule_constructor(1, &l2, &l1, 16, false),
    *r2 = rule_constructor(1, &l3, &l4, 15, false);

    size_t total_hidden, total_recovered, total_incorrectly_recovered, total_not_recovered;
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, &total_hidden, &total_recovered,
    &total_incorrectly_recovered, &total_not_recovered), 0);
    ck_assert_int_ne(total_hidden, 0);
    ck_assert_int_eq(total_recovered, 0);
    ck_assert_int_eq(total_incorrectly_recovered, 0);
    ck_assert_int_eq(total_not_recovered, total_hidden);

    knowledge_base_add_rule(nerd->knowledge_base, &r1);
    knowledge_base_add_rule(nerd->knowledge_base, &r2);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, &total_hidden, &total_recovered,
    &total_incorrectly_recovered, &total_not_recovered), 0);
    ck_assert_int_ne(total_hidden, 0);
    ck_assert_int_ge(total_recovered, 0);
    ck_assert_int_ge(total_incorrectly_recovered, 0);
    ck_assert_int_le(total_not_recovered, total_hidden);

    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, &total_hidden, &total_recovered,
    &total_incorrectly_recovered, NULL), 0);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, &total_hidden, &total_recovered,
    NULL, &total_not_recovered), 0);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, &total_hidden, &total_recovered,
    NULL, NULL), 0);

    ck_assert_int_eq(evaluate_random_literals(NULL, DATASET2, 0.3, &total_hidden, &total_recovered,
    NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, NULL, 0.3, &total_hidden, &total_recovered,
    NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0, &total_hidden, &total_recovered,
    NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 1.01, &total_hidden, &total_recovered,
    NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, -0.01, &total_hidden, &total_recovered,
    NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, NULL, &total_recovered, NULL,
    NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, &total_hidden, NULL, NULL, NULL),
    -1);

    nerd_destructor(&nerd);
    ck_assert_int_eq(evaluate_random_literals(nerd, DATASET2, 0.3, &total_hidden, &total_recovered,
    NULL, NULL), -1);
}
END_TEST

START_TEST(one_specific_literal_evaluation_test) {
    Nerd *nerd = nerd_constructor(DATASET2, ',', true, true, 15.0, 3, 50, 1, 1.5, 4.5, 1);
    Literal *l1 = literal_constructor("animal_bat", true),
    *l2 = literal_constructor("flies?_yes", true), *l3 = literal_constructor("animal_human", true),
    *l4 = literal_constructor("flies?_no", true);
    Rule *r1 = rule_constructor(1, &l1, &l2, 16, false),
    *r2 = rule_constructor(1, &l3, &l4, 15, false);
    Context *literals_to_evaluate = context_constructor(false);
    context_add_literal(literals_to_evaluate, &l2);

    float accuracy, abstain_ratio;
    ck_assert_int_eq(evaluate_one_specific_literal(nerd, DATASET2, literals_to_evaluate, &accuracy,
    &abstain_ratio), 1);

    context_add_literal(literals_to_evaluate, &l4);
    ck_assert_int_eq(evaluate_one_specific_literal(nerd, DATASET2, literals_to_evaluate, &accuracy,
    &abstain_ratio), 0);
    ck_assert_float_eq(accuracy, 0);
    ck_assert_float_eq(abstain_ratio, 1);

    knowledge_base_add_rule(nerd->knowledge_base, &r1);
    knowledge_base_add_rule(nerd->knowledge_base, &r2);
    ck_assert_int_eq(evaluate_one_specific_literal(nerd, DATASET2, literals_to_evaluate, &accuracy,
    &abstain_ratio), 0);
    ck_assert_float_eq(accuracy, 0.5);
    ck_assert_float_eq(abstain_ratio, 0.5);

    ck_assert_int_eq(evaluate_one_specific_literal(nerd, DATASET2, literals_to_evaluate, &accuracy, NULL),
    0);
    ck_assert_int_eq(evaluate_one_specific_literal(NULL, DATASET2, literals_to_evaluate, &accuracy, NULL),
    -1);
    ck_assert_int_eq(evaluate_one_specific_literal(nerd, NULL, literals_to_evaluate, &accuracy, NULL), -1);
    ck_assert_int_eq(evaluate_one_specific_literal(nerd, DATASET2, NULL, &accuracy, NULL), -1);
    ck_assert_int_eq(evaluate_one_specific_literal(nerd, DATASET2, literals_to_evaluate, NULL, NULL), -1);

    nerd_destructor(&nerd);
    ck_assert_int_eq(evaluate_one_specific_literal(nerd, DATASET2, literals_to_evaluate, &accuracy, NULL),
    -1);


    context_destructor(&literals_to_evaluate);
}
END_TEST

Suite *metrics_suite() {
    Suite *suite;
    TCase *evaluation_case;

    suite = suite_create("Metrics");
    evaluation_case = tcase_create("Evaluation Case");
    tcase_add_test(evaluation_case, all_literals_evaluation_test);
    tcase_add_test(evaluation_case, random_literals_evaluation_test);
    tcase_add_test(evaluation_case, one_specific_literal_evaluation_test);
    suite_add_tcase(suite, evaluation_case);

    return suite;
}

int main() {
    Suite *suite = metrics_suite();
    SRunner *s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
