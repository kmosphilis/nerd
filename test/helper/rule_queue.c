#include <malloc.h>
#include <check.h>

#include "rule_queue.h"
#include "rule.h"

/**
 * @brief Creates a dynamic Rule array.
 *
 * @return The Rule array. Use destruct_rules() to deallocate this array.
 */
Rule *create_rules() {
    Rule *rules = (Rule *) malloc(sizeof(Rule) * RULES_TO_CREATE);
    unsigned int j;

    const size_t body_size[3] = {3, 2, 4};

    char *body_literal_atoms[3][4] = {
        {"Penguin", "Bird", "Antarctica"},
        {"Albatross", "Bird"},
        {"Seagull", "Bird", "Harbor", "Ocean"}
    };
    int body_literal_signs[3][4] = {
        {1, 1, 1},
        {1, 1},
        {1, 1, 1, 1}
    };

    int head_sign[3] = {0, 1, 1};
    float starting_weight = 0;

    for (j = 0; j < RULES_TO_CREATE; ++j) {
        Rule rule;
        Literal *body = (Literal *) malloc(sizeof(Literal) * body_size[j]);
        Literal head;

        unsigned int i;
        for (i = 0; i < body_size[j]; ++i) {
            Literal l;
            literal_constructor(&l, body_literal_atoms[j][i], body_literal_signs[j][i]);
            body[i] = l;
        }

        literal_constructor(&head, "Fly", head_sign[j]);

        rule_constructor(&rule, body_size[j], &body, &head, starting_weight);

        literal_destructor(&head);

        for (i = 0; i < body_size[j]; ++i) {
            literal_destructor(&body[i]);
        }

        free(body);

        rules[j] = rule;
        starting_weight += 1;
    }
    return rules;
}

/**
 * @brief Destructs the dynamic Rule array created from create_rules().
 *
 * @param rules The dynamic Rule array.
 */
void destruct_rules(Rule * const rules) {
    unsigned int i;
    for(i = 0; i < RULES_TO_CREATE; ++i) {
        rule_destructor(&(rules[i]));
    }
    free(rules);
}

/**
 * @brief Creates a RuleQueue by constructing and filling it with values.
 *
 * @param rule_queue The RuleQueue to be created.
 */
void create_rule_queue(RuleQueue * const rule_queue) {
    
    rule_queue_constructor(rule_queue);

    Rule *rules = create_rules();

    unsigned int i;
    for (i = 0; i < RULES_TO_CREATE; ++i) {
        rule_queue_enqueue(rule_queue, &(rules[i]));
    }

    destruct_rules(rules);
}
