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

/**
 * @brief Check two RuleQueues to determine if they are equal (rule_queue1 == rule_queue2).
 * 
 * @param rule_queue1 The first RuleQueue to compare.
 * @param rule_queue2 The second RuleQueue to compare.
 */
void ck_assert_rule_queue_eq(const RuleQueue * const rule_queue1,
const RuleQueue * const rule_queue2) {
    ck_assert_int_eq(rule_queue1->length, rule_queue2->length);

    unsigned int i;

    for (i = 0; i < rule_queue1->length; ++i) {
        ck_assert_rule_eq(&(rule_queue1->rules[i]), &(rule_queue2->rules[i]));
    }
}

/**
 * @brief Check if a RuleQueue is not empty.
 * 
 * @param rule_queue1 The RuleQueue to check.
 */
void ck_assert_rule_queue_notempty(const RuleQueue * const rule_queue) {
    ck_assert_int_ne(rule_queue->length, 0);
    ck_assert_ptr_nonnull(rule_queue->rules);
}

/**
 * @brief Check if a RuleQueue is empty.
 * 
 * @param rule_queue1 The RuleQueue to check.
 */
void ck_assert_rule_queue_empty(const RuleQueue * const rule_queue) {
    ck_assert_int_eq(rule_queue->length, 0);
    ck_assert_ptr_null(rule_queue->rules);
}