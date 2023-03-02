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
        {"Seagull", "Bird", "Harbor", "Ocean"}
    };
    const bool body_literal_signs[RULES_TO_CREATE][4] = {
        {true, true, true},
        {true, true},
        {true, true, true, true}
    };

    int head_sign[RULES_TO_CREATE] = {0, 1, 1};
    float starting_weight = 0;

    for (j = 0; j < RULES_TO_CREATE; ++j) {
        Literal *body[RULES_TO_CREATE];
        Literal *head;

        unsigned int i;
        for (i = 0; i < body_size[j]; ++i) {
            body[i] = literal_constructor(body_literal_atoms[j][i], body_literal_signs[j][i]);
        }

        head = literal_constructor("Fly", head_sign[j]);

        rules[j] = rule_constructor(body_size[j], body, &head, starting_weight);
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
 *
 * @return A * to the created RuleQueue.
 */
RuleQueue *create_rule_queue() {
    RuleQueue *rule_queue = rule_queue_constructor();

    Rule **rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, &rules[i]);
    }

    destruct_rules(rules);
    return rule_queue;
}
