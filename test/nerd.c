#include <check.h>
#include <stdlib.h>

#include "../src/nerd.h"
#include "helper/rule_queue.h"

START_TEST(construct_destruct_test) {
    Nerd *nerd = NULL;
    KnowledgeBase *knowledge_base = NULL;
    RuleQueue *rule_queue = create_rule_queue();

    unsigned int i, rule_queue_length = rule_queue->length;
    for (i = 0; i < rule_queue_length; ++i) {
        Rule *rule = NULL;
        rule_copy(&rule, rule_queue->rules[i]);
        rule_promote(rule, 3.0);
        rule_queue_enqueue(rule_queue, rule);
        // rule_queue_enqueue(&rule_queue, &rule);
        rule_destructor(&rule);
    }

    knowledge_base = knowledge_base_constructor(3.0);

    for (i = 0; i < rule_queue->length; ++i) {
        knowledge_base_add_rule(knowledge_base, rule_queue->rules[i]);
    }

    rule_queue_destructor(&rule_queue);

    nerd = nerd_constructor("../data/test.txt", 1, 10.0, 3, 100, 1000, 0.5, 1.5, 0);

    // knowledge_base_copy(&(nerd.knowledge_base), &knowledge_base);
    knowledge_base_destructor(&knowledge_base);

    nerd_start_learning(nerd);

    nerd_destructor(&nerd);
}

Suite *nerd_suite() {
    Suite *suite;
    TCase *create_case;

    suite = suite_create("Nerd");
    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

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