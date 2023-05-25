#include <stdlib.h>
#include <check.h>
#include <stdbool.h>

#include "helper/nerd_utils.h"
#include "helper/knowledge_base.h"
#include "helper/rule_queue.h"
#include "helper/rule.h"
#include "../src/knowledge_base.h"
#include "../src/rule_hypergraph.h"

START_TEST(construct_destruct_test) {
    KnowledgeBase *knowledge_base = knowledge_base_constructor(5.0, true);

    ck_assert_float_eq_tol(knowledge_base->activation_threshold, 5.0, 0.00001);
    ck_assert_knowledge_base_empty(knowledge_base);
    ck_assert_rule_queue_empty(knowledge_base->active);
    ck_assert_rule_hypergraph_empty(knowledge_base->hypergraph);

    knowledge_base_destructor(&knowledge_base);
    ck_assert_ptr_null(knowledge_base);

    knowledge_base_destructor(&knowledge_base);
}
END_TEST

START_TEST(add_rule_test) {
    KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, true);
    RuleQueue *rule_queue1 = create_rule_queue1(), *rule_queue2 = NULL,
    *rule_queue3 = rule_queue_constructor(true),
    *inactive_rules;


    unsigned int i;
    for (i = 0; i < rule_queue1->length; ++i) {
        Rule *rule;
        rule_copy(&rule, rule_queue1->rules[i]);
        rule->weight += 3;
        rule_queue_enqueue(rule_queue3, &rule);
    }

    for (i = 0; i < rule_queue1->length; ++i) {
        ck_assert_int_eq(knowledge_base_add_rule(knowledge_base, &(rule_queue1->rules[i])), 1);
        ck_assert_int_eq(knowledge_base_add_rule(knowledge_base, &(rule_queue3->rules[i])), 0);
    }
    rule_queue_copy(&rule_queue2, rule_queue1);
    rule_queue_destructor(&rule_queue1);
    rule_queue_destructor(&rule_queue3);

    ck_assert_knowledge_base_notempty(knowledge_base);
    ck_assert_rule_queue_empty(knowledge_base->active);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_rule_queue_eq(inactive_rules, rule_queue2);

    size_t old_actives = knowledge_base->active->length,
    old_inactives = inactive_rules->length;
    rule_queue_destructor(&inactive_rules);
    ck_assert_int_eq(knowledge_base_add_rule(knowledge_base, NULL), -1);
    ck_assert_int_eq(knowledge_base->active->length, old_actives);
    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, old_inactives);
    rule_queue_destructor(&inactive_rules);
    ck_assert_int_eq(knowledge_base_add_rule(knowledge_base, &(rule_queue2->rules[0])), 0);

    rule_queue3 = create_rule_queue2();
    rule_queue_destructor(&rule_queue2);

    for (i = 0; i < rule_queue3->length; ++i) {
        ck_assert_int_eq(knowledge_base_add_rule(knowledge_base, &(rule_queue3->rules[i])), 1);
    }
    rule_queue_copy(&rule_queue2, rule_queue3);
    rule_queue_destructor(&rule_queue3);

    rule_hypergraph_get_inactive_rules(knowledge_base, &inactive_rules);
    ck_assert_int_eq(inactive_rules->length, old_inactives);
    rule_queue_destructor(&inactive_rules);
    ck_assert_int_ne(knowledge_base->active->length, old_actives);
    ck_assert_rule_queue_eq(knowledge_base->active, rule_queue2);

    knowledge_base_destructor(&knowledge_base);
    ck_assert_ptr_null(knowledge_base);
    ck_assert_int_eq(knowledge_base_add_rule(knowledge_base, &(rule_queue2->rules[0])), -1);
    ck_assert_ptr_null(knowledge_base);

    rule_queue_destructor(&rule_queue2);
}
END_TEST

START_TEST(copy_test) {
    KnowledgeBase *knowledge_base1 = knowledge_base_constructor(3.0, true), *knowledge_base2 = NULL;
    RuleQueue *rule_queue1 = create_rule_queue1(), *rule_queue2 = create_rule_queue2();

    unsigned int i;
    for (i = 0; i < rule_queue1->length; ++i) {
        knowledge_base_add_rule(knowledge_base1, &(rule_queue1->rules[i]));
    }
    rule_queue_destructor(&rule_queue1);

    knowledge_base_copy(&knowledge_base2, knowledge_base1);
    ck_assert_knowledge_base_eq(knowledge_base1, knowledge_base2);
    ck_assert_ptr_ne(knowledge_base1, knowledge_base2);
    ck_assert_ptr_ne(knowledge_base1->active, knowledge_base2->active);
    ck_assert_ptr_ne(knowledge_base1->hypergraph, knowledge_base2->hypergraph);

    for (i = 0; i < rule_queue2->length; ++i) {
        knowledge_base_add_rule(knowledge_base1, &(rule_queue2->rules[i]));
    }
    rule_queue_destructor(&rule_queue2);

    ck_assert_int_ne(knowledge_base1->active->length, knowledge_base2->active->length);

    knowledge_base_destructor(&knowledge_base2);
    knowledge_base_copy(&knowledge_base2, knowledge_base1);
    ck_assert_knowledge_base_eq(knowledge_base1, knowledge_base2);

    knowledge_base_destructor(&knowledge_base1);
    ck_assert_ptr_null(knowledge_base1);
    ck_assert_knowledge_base_notempty(knowledge_base2);

    knowledge_base_copy(&knowledge_base1, NULL);
    ck_assert_ptr_null(knowledge_base1);

    knowledge_base_destructor(&knowledge_base2);
}
END_TEST

START_TEST(create_new_rules_test) {
    KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, true);
    RuleQueue *rule_queue = create_rule_queue2(), *result;
    Scene *observed = scene_constructor(true), *inferred = scene_constructor(true);
    Literal *literal = literal_constructor("Penguin", 1), *copy;
    scene_add_literal(observed, &literal);

    literal = literal_constructor("Antarctica", 1);
    literal_copy(&copy, literal);
    scene_add_literal(observed, &literal);
    scene_add_literal(inferred, &copy);

    literal = literal_constructor("Bird", 1);
    literal_copy(&copy, literal);
    scene_add_literal(observed, &literal);
    scene_add_literal(inferred, &copy);

    literal = literal_constructor("Fly", 0);
    scene_add_literal(observed, &literal);

    literal = literal_constructor("Albatross", 1);
    scene_add_literal(observed, &literal);

    unsigned int i;
    for (i = 0; i < rule_queue->length; ++i) {
        knowledge_base_add_rule(knowledge_base, &(rule_queue->rules[i]));
    }
    rule_queue_destructor(&rule_queue);

    size_t old_size, new_size;
    rule_hypergraph_get_inactive_rules(knowledge_base, &result);
    old_size = knowledge_base->active->length + result->length;
    rule_queue_destructor(&result);
    do {
        knowledge_base_create_new_rules(knowledge_base, observed, inferred, 3, 5);
        rule_hypergraph_get_inactive_rules(knowledge_base, &result);
        new_size = knowledge_base->active->length + result->length;
        rule_queue_destructor(&result);
    } while (old_size == new_size);
    ck_assert_int_ne(old_size, new_size);

    old_size = new_size;
    do {
        knowledge_base_create_new_rules(knowledge_base, observed, inferred, 3, 5);
        rule_hypergraph_get_inactive_rules(knowledge_base, &result);
        new_size = knowledge_base->active->length + result->length;
        rule_queue_destructor(&result);
    } while (old_size == new_size);
    ck_assert_int_ne(old_size, new_size);

    old_size = new_size;
    knowledge_base_create_new_rules(knowledge_base, NULL, inferred, 3, 5);
    knowledge_base_create_new_rules(knowledge_base, NULL, inferred, 3, 5);
    rule_hypergraph_get_inactive_rules(knowledge_base, &result);
    new_size = knowledge_base->active->length + result->length;
    rule_queue_destructor(&result);
    ck_assert_int_eq(old_size, new_size);

    knowledge_base_create_new_rules(knowledge_base, observed, NULL, 3, 5);
    knowledge_base_create_new_rules(knowledge_base, observed, NULL, 3, 5);
    rule_hypergraph_get_inactive_rules(knowledge_base, &result);
    new_size = knowledge_base->active->length + result->length;
    rule_queue_destructor(&result);
    ck_assert_int_ne(old_size, new_size);

    knowledge_base_destructor(&knowledge_base);
    scene_destructor(&inferred);
    scene_destructor(&observed);
}
END_TEST

START_TEST(to_string_test) {
    KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, true);
    RuleQueue *rule_queue1 = create_rule_queue1(), *rule_queue2 = create_rule_queue2();

    unsigned int i;
    for (i = 0; i < rule_queue1->length; ++i) {
        knowledge_base_add_rule(knowledge_base, &(rule_queue1->rules[i]));
    }
    rule_queue_destructor(&rule_queue1);

    char *knowledge_base_string = knowledge_base_to_string(knowledge_base);

    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: 3.0000\n"
        "Active Rules: [\n]\n"
        "Inactive Rules: [\n"
        "\t(penguin, bird, antarctica) => -fly (0.0000),\n"
        "\t(albatross, bird) => fly (1.0000),\n"
        "\t(seagull, bird, harbour, ocean) => fly (2.0000)\n]");
    free(knowledge_base_string);

    for (i = 0; i < rule_queue2->length; ++i) {
        knowledge_base_add_rule(knowledge_base, &(rule_queue2->rules[i]));
    }
    rule_queue_destructor(&rule_queue2);

    knowledge_base_string = knowledge_base_to_string(knowledge_base);
    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: 3.0000\n"
        "Active Rules: [\n"
        "\t(penguin, bird) => -fly (5.0000),\n"
        "\t(eagle, bird) => fly (6.0000),\n"
        "\t(bat, mammal, cave, -bird) => fly (7.0000)\n]\n"
        "Inactive Rules: [\n"
        "\t(penguin, bird, antarctica) => -fly (0.0000),\n"
        "\t(albatross, bird) => fly (1.0000),\n"
        "\t(seagull, bird, harbour, ocean) => fly (2.0000)\n]");
    free(knowledge_base_string);

    rule_queue_destructor(&(knowledge_base->active));
    knowledge_base_string = knowledge_base_to_string(knowledge_base);
    ck_assert_pstr_eq(knowledge_base_string, NULL);

    knowledge_base->active = rule_queue_constructor(false);
    knowledge_base_string = knowledge_base_to_string(knowledge_base);
    ck_assert_str_eq(knowledge_base_string, "Knowledge Base:\n"
        "Activation Threshold: 3.0000\n"
        "Active Rules: [\n]\n"
        "Inactive Rules: [\n"
        "\t(penguin, bird, antarctica) => -fly (0.0000),\n"
        "\t(albatross, bird) => fly (1.0000),\n"
        "\t(seagull, bird, harbour, ocean) => fly (2.0000)\n]");
    free(knowledge_base_string);

    knowledge_base_destructor(&knowledge_base);
    knowledge_base_string = knowledge_base_to_string(knowledge_base);
    ck_assert_pstr_eq(knowledge_base_string, NULL);
}
END_TEST

START_TEST(to_prudensjs_test) {
    KnowledgeBase *knowledge_base = knowledge_base_constructor(3.0, true);
    RuleQueue *rule_queue1 = create_rule_queue1(), *rule_queue2 = create_rule_queue2();

    unsigned int i;
    for (i = 0; i < rule_queue1->length; ++i) {
        knowledge_base_add_rule(knowledge_base, &(rule_queue1->rules[i]));
    }
    rule_queue_destructor(&rule_queue1);

    const char * const expected_empty_result = "{\"type\": \"output\", \"kb\": [], \"code\": \"\", "
    "\"imports\": \"\", \"warnings\": [], \"customPriorities\": []}";

    const char * const expected_result = "{\"type\": \"output\", \"kb\": [{\"name\": \"Rule2\", "
    "\"body\": [{\"name\": \"bat\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"mammal\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}, {\"name\": \"cave\", \"sign\": true, \"isJS\": false, "
    "\"isEquality\": false, \"isInEquality\": false, \"isAction\": false, \"arity\": 0}, "
    "{\"name\": \"bird\", \"sign\": false, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}], \"head\": {\"name\": \"fly\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}}, {\"name\": \"Rule1\", \"body\": ["
    "{\"name\": \"eagle\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}, {\"name\": \"bird\", "
    "\"sign\": true, \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
    "\"isAction\": false, \"arity\": 0}], \"head\": {\"name\": \"fly\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}}, {\"name\": \"Rule0\", \"body\": [{\"name\": \"penguin\", \"sign\": true, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}, {\"name\": \"bird\", \"sign\": true, \"isJS\": false, \"isEquality\": false, "
    "\"isInEquality\": false, \"isAction\": false, \"arity\": 0}], \"head\": {\"name\": \"fly\", \"sign\": false, "
    "\"isJS\": false, \"isEquality\": false, \"isInEquality\": false, \"isAction\": false, "
    "\"arity\": 0}}], \"code\": \"\", \"imports\": \"\", \"warnings\": [], "
    "\"customPriorities\": []}";

    char *knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(knowledge_base);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, expected_empty_result);
    free(knowledge_base_prudensjs_string);

    for (i = 0; i < rule_queue2->length; ++i) {
        knowledge_base_add_rule(knowledge_base, &(rule_queue2->rules[i]));
    }
    rule_queue_destructor(&rule_queue2);

    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(knowledge_base);
    ck_assert_str_eq(knowledge_base_prudensjs_string, expected_result);
    free(knowledge_base_prudensjs_string);

    rule_queue_destructor(&(knowledge_base->active));
    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(knowledge_base);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, NULL);

    knowledge_base->active = rule_queue_constructor(true);
    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(knowledge_base);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, expected_empty_result);
    free(knowledge_base_prudensjs_string);

    knowledge_base_destructor(&knowledge_base);
    knowledge_base_prudensjs_string = knowledge_base_to_prudensjs(knowledge_base);
    ck_assert_pstr_eq(knowledge_base_prudensjs_string, NULL);
}
END_TEST

Suite *knowledge_base_suite() {
    Suite *suite;
    TCase *create_case, *add_rule_case, *copy_case, *create_new_rules_case, *convert_case;
    suite = suite_create("Knowledge Base");

    create_case = tcase_create("Create");
    tcase_add_test(create_case, construct_destruct_test);
    suite_add_tcase(suite, create_case);

    add_rule_case = tcase_create("Add Rule");
    tcase_add_test(add_rule_case, add_rule_test);
    suite_add_tcase(suite, add_rule_case);

    copy_case = tcase_create("Copy");
    tcase_add_test(copy_case, copy_test);
    suite_add_tcase(suite, copy_case);

    create_new_rules_case = tcase_create("Create New Rules");
    tcase_add_test(create_new_rules_case, create_new_rules_test);
    suite_add_tcase(suite, create_new_rules_case);

    convert_case = tcase_create("Conversion");
    tcase_add_test(convert_case, to_string_test);
    tcase_add_test(convert_case, to_prudensjs_test);
    suite_add_tcase(suite, convert_case);

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
