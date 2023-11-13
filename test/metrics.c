#include <check.h>

#include "../src/nerd.h"
#include "../src/nerd_helper.h"
#include "../src/knowledge_base.h"
#include "../src/context.h"
#include "../src/metrics.h"

#define DATASET1 "../test/data/sensor_test1.txt"
#define DATASET2 "../test/data/sensor_test3.txt"

PrudensSettings_ptr settings = NULL;

START_TEST(all_literals_evaluation_test) {
    Nerd *nerd =
    nerd_constructor(15.0, 5, 3, 50, 1.5, 4.5, true, true);
    Literal *l1 = literal_constructor("penguin", true), *l2 = literal_constructor("fly", false),
    *l3 = literal_constructor("bird", true);
    Rule *r1 = rule_constructor(1, &l1, &l2, 15.0, false),
    *r2 = rule_constructor(1, &l3, &l1, 16.0, false);

    FILE *file = fopen(DATASET1, "r");
    if (!file) {
        ck_abort_msg("%s does not exist.", DATASET1);
    }
    char buffer[BUFFER_SIZE];
    size_t total_read_attributes = 0;
    while (fscanf(file, "%s", buffer) != EOF) {
        ++total_read_attributes;
    }
    fclose(file);

    Sensor *sensor = sensor_constructor_from_file(DATASET1, ' ', true, false);

    size_t total_hidden, total_recovered, total_incorrectly_recovered, total_not_recovered;
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, sensor, &total_hidden,
    &total_recovered, &total_incorrectly_recovered, &total_not_recovered), 0);
    ck_assert_int_ne(total_hidden, 0);
    ck_assert_int_eq(total_hidden, total_read_attributes);
    ck_assert_int_eq(total_recovered, 0);
    ck_assert_int_eq(total_incorrectly_recovered, 0);
    ck_assert_int_eq(total_not_recovered, total_hidden);

    knowledge_base_add_rule(nerd->knowledge_base, &r1);
    knowledge_base_add_rule(nerd->knowledge_base, &r2);
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, sensor, &total_hidden,
    &total_recovered, &total_incorrectly_recovered, &total_not_recovered), 0);
    ck_assert_int_eq(total_hidden, total_read_attributes);
    ck_assert_int_gt(total_recovered, 0);
    ck_assert_int_ge(total_incorrectly_recovered, 0);
    ck_assert_int_lt(total_not_recovered, total_hidden);

    size_t old_hidden = total_hidden, old_recovered = total_recovered;
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, sensor, &total_hidden,
    &total_recovered, NULL, NULL), 0);
    ck_assert_int_eq(old_hidden, total_hidden);
    ck_assert_int_eq(old_recovered, total_recovered);

    ck_assert_int_eq(evaluate_all_literals(NULL, settings, sensor, &total_hidden,
    &total_recovered, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, NULL, &total_hidden, &total_recovered,
    NULL, NULL), -1);
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, sensor, NULL, &total_recovered, NULL,
    NULL), -1);
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, sensor, &total_hidden, NULL, NULL,
    NULL), -1);

    sensor_destructor(&sensor);
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, sensor, &total_hidden,
    &total_recovered, NULL, NULL), -1);

    nerd_destructor(&nerd);
    ck_assert_int_eq(evaluate_all_literals(nerd, settings, sensor, &total_hidden,
    &total_recovered, NULL, NULL), -1);
}
END_TEST

START_TEST(random_literals_evaluation_test) {
    Nerd *nerd =
    nerd_constructor(15.0, 5, 3, 50, 1.5, 4.5, true, true);
    Literal *l1 = literal_constructor("class_bird", true),
    *l2 = literal_constructor("flies?_yes", true), *l3 = literal_constructor("class_mammal", true),
    *l4 = literal_constructor("flies?_no", true);
    Rule *r1 = rule_constructor(1, &l2, &l1, 16, false),
    *r2 = rule_constructor(1, &l3, &l4, 15, false);
    const float ratio = 0.5;

    Sensor *sensor = sensor_constructor_from_file(DATASET2, ',', true, true);

    size_t total_hidden, total_recovered, total_incorrectly_recovered, total_not_recovered;
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, &total_hidden,
    &total_recovered, &total_incorrectly_recovered, &total_not_recovered), 0);
    ck_assert_int_ne(total_hidden, 0);
    ck_assert_int_eq(total_recovered, 0);
    ck_assert_int_eq(total_incorrectly_recovered, 0);
    ck_assert_int_eq(total_not_recovered, total_hidden);

    knowledge_base_add_rule(nerd->knowledge_base, &r1);
    knowledge_base_add_rule(nerd->knowledge_base, &r2);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, &total_hidden,
    &total_recovered, &total_incorrectly_recovered, &total_not_recovered), 0);
    ck_assert_int_ne(total_hidden, 0);
    ck_assert_int_ge(total_recovered, 0);
    ck_assert_int_ge(total_incorrectly_recovered, 0);
    ck_assert_int_le(total_not_recovered, total_hidden);

    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, &total_hidden,
    &total_recovered, &total_incorrectly_recovered, NULL), 0);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, &total_hidden,
    &total_recovered, NULL, &total_not_recovered), 0);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, &total_hidden,
    &total_recovered, NULL, NULL), 0);

    ck_assert_int_eq(evaluate_random_literals(NULL, settings, sensor, ratio, &total_hidden,
    &total_recovered, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, NULL, ratio, &total_hidden,
    &total_recovered, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, 0, &total_hidden,
    &total_recovered, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, 1.01, &total_hidden,
    &total_recovered, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, -0.01, &total_hidden,
    &total_recovered, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, NULL,
    &total_recovered, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, &total_hidden, NULL,
    NULL, NULL), -1);

    sensor_destructor(&sensor);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio,
    &total_hidden, &total_recovered, NULL, NULL), -1);

    nerd_destructor(&nerd);
    ck_assert_int_eq(evaluate_random_literals(nerd, settings, sensor, ratio, &total_hidden,
    &total_recovered, NULL, NULL), -1);
}
END_TEST

START_TEST(one_specific_literal_evaluation_test) {
    Nerd *nerd =
    nerd_constructor(5.0, 5, 3, 50, 1.5, 4.5, true, true);
    Literal *l1 = literal_constructor("animal_bat", true),
    *l2 = literal_constructor("flies?_yes", true), *l3 = literal_constructor("animal_human", true),
    *l4 = literal_constructor("flies?_no", true);
    Rule *r1 = rule_constructor(1, &l1, &l2, 16, false),
    *r2 = rule_constructor(1, &l3, &l4, 15, false);
    Context *literals_to_evaluate = context_constructor(false);
    context_add_literal(literals_to_evaluate, &l2);

    Sensor *sensor = sensor_constructor_from_file(DATASET2, ',', true, true);

    float accuracy, abstain_ratio;
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, &accuracy,
    &abstain_ratio, NULL, NULL, NULL, NULL), 1);

    context_add_literal(literals_to_evaluate, &l4);
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, &accuracy,
    &abstain_ratio, NULL, NULL, NULL, NULL), 0);
    ck_assert_float_eq(accuracy, 0);
    ck_assert_float_eq(abstain_ratio, 1);

    knowledge_base_add_rule(nerd->knowledge_base, &r1);
    knowledge_base_add_rule(nerd->knowledge_base, &r2);
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, &accuracy,
    &abstain_ratio, NULL, NULL, NULL, NULL), 0);
    ck_assert_float_eq(accuracy, 0.5);
    ck_assert_float_eq(abstain_ratio, 0.5);

    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, &accuracy, NULL,
    NULL, NULL, NULL, NULL), 0);
    accuracy = 0;
    abstain_ratio = 0;
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, &accuracy,
    &abstain_ratio, NULL, NULL, NULL, NULL), 0);
    ck_assert_float_eq(accuracy, 0.5);
    ck_assert_float_eq(abstain_ratio, 0.5);
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, NULL, NULL, NULL,
    NULL, NULL, NULL), 0);

    size_t total_observations;
    Scene **inferences = NULL;
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, NULL, NULL,
    &total_observations, NULL, &inferences, NULL), 0);
    ck_assert_int_ne(total_observations, 0);
    ck_assert_ptr_nonnull(inferences);
    unsigned int i;
    for (i = 0; i < total_observations; ++i) {
        ck_assert_ptr_nonnull(inferences[i]);
        scene_destructor(&(inferences[i]));
    }
    safe_free(inferences);

    const size_t old_observations = total_observations;
    total_observations = 0;
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, NULL, NULL, NULL,
    NULL, &inferences, NULL), -2);

    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, NULL, NULL,
    &total_observations, NULL, NULL, NULL), 0);
    ck_assert_int_eq(old_observations, total_observations);

    ck_assert_int_eq(evaluate_labels(NULL, settings, sensor, literals_to_evaluate, &accuracy, NULL,
    NULL, NULL, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_labels(nerd, settings, NULL, literals_to_evaluate, &accuracy, NULL,
    NULL, NULL, NULL, NULL), -1);
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, NULL, &accuracy, NULL, NULL, NULL,
    NULL, NULL), -1);

    sensor_destructor(&sensor);
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, &accuracy,
    &abstain_ratio, NULL, NULL, NULL, NULL), -1);

    nerd_destructor(&nerd);
    ck_assert_int_eq(evaluate_labels(nerd, settings, sensor, literals_to_evaluate, &accuracy, NULL,
    NULL, NULL, NULL, NULL), -1);

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

int main(int argc, char *argv[]) {
    prudensjs_settings_constructor(&settings, argv[0], NULL, NULL, NULL);
    Suite *suite = metrics_suite();
    SRunner *s_runner;

    s_runner = srunner_create(suite);
    srunner_set_fork_status(s_runner, CK_NOFORK);

    srunner_run_all(s_runner, CK_ENV);
    int number_failed = srunner_ntests_failed(s_runner);
    srunner_free(s_runner);
    prudensjs_settings_destructor(&settings);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
