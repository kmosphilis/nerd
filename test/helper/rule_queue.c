#include <check.h>
#include <stdlib.h>
#include <stdbool.h>

#include "rule_queue.h"
#include "rule.h"

/**
 * @brief Creates a dynamic Rule array.
 *
 * @return The Rule array. Use destruct_rules() to deallocate this array.
 */
Rule **create_rules() {
    Rule **rules = (Rule **) malloc(sizeof(Rule *) * RULES_TO_CREATE);
    unsigned int j;

    const size_t body_size[RULES_TO_CREATE] = {3, 2, 4};

    const char *body_literal_atoms[RULES_TO_CREATE][4] = {
        {"Penguin", "Bird", "Antarctica"},
        {"Albatross", "Bird"},
        {"Seagull", "Bird", "Harbour", "Ocean"}
    };
    const bool body_literal_signs[RULES_TO_CREATE][4] = {
        {true, true, true},
        {true, true},
        {true, true, true, true}
    };

    const bool head_sign[RULES_TO_CREATE] = {false, true, true};
    float starting_weight = 0;

    for (j = 0; j < RULES_TO_CREATE; ++j) {
        Literal *body[RULES_TO_CREATE];
        Literal *head;

        unsigned int i;
        for (i = 0; i < body_size[j]; ++i) {
            body[i] = literal_constructor(body_literal_atoms[j][i], body_literal_signs[j][i]);
        }

        head = literal_constructor("Fly", head_sign[j]);

        rules[j] = rule_constructor(body_size[j], body, &head, starting_weight, true);
        starting_weight += 1;
    }
    return rules;
}

/**
 * @brief Destructs the dynamic Rule array created from create_rules().
 *
 * @param rules The dynamic Rule array.
 */
void destruct_rules(Rule **rules) {
    unsigned int i;
    for(i = 0; i < RULES_TO_CREATE; ++i) {
        rule_destructor(&(rules[i]));
    }
    free(rules);
}

/**
 * @brief Creates a RuleQueue by constructing and filling it with values.
 * The Rules created are: (penguin, bird, antarctica) => -fly (0.0000),
 *                        (albatross, bird) => fly (1.0000),
 *                        (seagull, bird, harbour, ocean) => fly (2.0000)
 *
 * @return A * to the created RuleQueue.
 */
RuleQueue *create_rule_queue1() {
    RuleQueue *rule_queue = rule_queue_constructor(false);

    Rule **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, &rules[i]);
    }

    free(rules);
    return rule_queue;
}

/**
 * @brief Creates a RuleQueue just like create_rule_queue1, with different rules and weights (> 4).
 * The Rules created are: (penguin, bird) => -fly (5.0000),
 *                        (eagle, bird) => fly (6.0000),
 *                        (bat, mammal, cave, bird) => (7.0000)
 *
 * @return A * to the created RuleQueue.
*/
RuleQueue *create_rule_queue2() {
    RuleQueue *rule_queue = rule_queue_constructor(false);
    unsigned int j;

    const size_t body_size[RULES_TO_CREATE] = {2, 2, 4};

    const char *body_literal_atoms[RULES_TO_CREATE][4] = {
        {"Penguin", "Bird"},
        {"Eagle", "Bird"},
        {"Bat", "Mammal", "Cave", "Bird"}
    };
    const bool body_literal_signs[RULES_TO_CREATE][4] = {
        {true, true},
        {true, true},
        {true, true, true, false}
    };

    const bool head_sign[RULES_TO_CREATE] = {false, true, true};
    float starting_weight = 5;

    for (j = 0; j < RULES_TO_CREATE; ++j) {
        Literal *body[RULES_TO_CREATE];
        Literal *head;

        unsigned int i;
        for (i = 0; i < body_size[j]; ++i) {
            body[i] = literal_constructor(body_literal_atoms[j][i], body_literal_signs[j][i]);
        }

        head = literal_constructor("Fly", head_sign[j]);

        Rule *rule = rule_constructor(body_size[j], body, &head, starting_weight, true);
        rule_queue_enqueue(rule_queue, &rule);

        starting_weight += 1;
    }

    return rule_queue;
}
