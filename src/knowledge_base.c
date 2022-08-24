#include <malloc.h>
#include <string.h>
#include <math.h>

#include "knowledge_base.h"

/**
 * @brief Constructs a KnowledgeBase.
 * 
 * @param knowledge_base The KnowledgeBase to be constructed. If NULL the process will fail.
 * @param activation_threshold The threshold which determines whether a rule should get activated or 
 * deactivated.
 */
void knowledge_base_construct(KnowledgeBase * const knowledge_base,
const float activation_threshold) {
    if (knowledge_base != NULL) {
        knowledge_base->activation_threshold = activation_threshold;
        rule_queue_construct(&(knowledge_base->active));
        rule_queue_construct(&(knowledge_base->inactive));
    }
}

/**
 * @brief Destructs a KnowledgeBase.
 * 
 * @param knowledge_base The KnowledgeBase to be destructed. If NULL the process will fail.
 */
void knowledge_base_destruct(KnowledgeBase * const knowledge_base) {
    if (knowledge_base != NULL) {
        rule_queue_destruct(&(knowledge_base->active));
        rule_queue_destruct(&(knowledge_base->inactive));
        knowledge_base->activation_threshold = INFINITY;
    }
}

/**
 * @brief Makes a copy of the given KnowledgeBase.
 * 
 * @param destination The KnowledgeBase to save the copy.
 * @param source The KnowledgeBase to be copied. If the KnowledgeBase is NULL, the contents of the 
 * destination will not be changed.
 */
void knowledge_base_copy(KnowledgeBase * const destination, const KnowledgeBase * const source) {
    if ((destination != NULL) && (source != NULL)) {
        destination->activation_threshold = source->activation_threshold;
        rule_queue_copy(&(destination->active), &(source->active));
        rule_queue_copy(&(destination->inactive), &(source->inactive));
    }
}

/**
 * @brief Adds a Rule in the KnowledgeBase. If the weight of the Rule is above the 
 * activation_threshold, the Rule will be added to the active RuleQueue, otherwise it will be added 
 * to the inactive RuleQueue.
 * 
 * @param knowledge_base The KnowledgeBase to be expanded.
 * @param rule The Rule to be added.
 */
void knowledge_base_add_rule(KnowledgeBase * const knowledge_base, const Rule * const rule) {
    if ((knowledge_base != NULL) && (rule != NULL)) {
        if (rule->weight >= knowledge_base->activation_threshold) {
            rule_queue_enqueue(&(knowledge_base->active), rule);
        } else {
            rule_queue_enqueue(&(knowledge_base->inactive), rule);
        }
    }
}

//TODO Create a Context struct.
/**
 * @brief Finds the applicaple rules from the given context (Literals).
 * 
 * @param knowledge_base The KnowledgeBase to find the Rules from.
 * @param literals The context.
 * @param literals_size The size of the context.
 * @param applicaple_rules The RuleQueue to save the applicaple Rules.
 */
void knowledge_base_applicable_rules(const KnowledgeBase * const knowledge_base,
Literal ** const literals, const unsigned int literals_size, RuleQueue * const applicaple_rules) {
    if ((knowledge_base != NULL) && (literals != NULL) && (applicaple_rules != NULL) && 
    (literals_size != 0)) {
        unsigned int i, j, k, literals_in_found_in_body = 0;
        for (i = 0; i < knowledge_base->active.length; ++i) {
            Rule *current_rule = &(knowledge_base->active.rules[i]);
            for (j = 0; j < current_rule->body_size; ++j) {
                for (k = 0; k < literals_size; ++k) {
                    if (literal_equals(&(current_rule->body[j]), &((*literals)[k]))) {
                        ++literals_in_found_in_body;
                        k = literals_size;
                    }
                }
            }
            if (literals_in_found_in_body == current_rule->body_size) {
                rule_queue_enqueue(applicaple_rules, current_rule);
            }
            literals_in_found_in_body = 0;
        }

        for (i = 0; i < knowledge_base->inactive.length; ++i) {
            Rule *current_rule = &(knowledge_base->inactive.rules[i]);
            for (j = 0; j < current_rule->body_size; ++j) {
                for (k = 0; k < literals_size; ++k) {
                    if (literal_equals(&(current_rule->body[j]), &((*literals)[k]))) {
                        ++literals_in_found_in_body;
                        k = literals_size;
                    }
                }
            }
            if (literals_in_found_in_body == current_rule->body_size) {
                rule_queue_enqueue(applicaple_rules, current_rule);
            }
            literals_in_found_in_body = 0;
        }
    }
}

/**
 * @brief Promotes all the given Rules in the KnowledgeBase.
 * 
 * @param knowledge_base The KnowledgeBase in which the given Rules should be promoted.
 * @param rules_to_promote The RuleQueue containing the Rules to be promoted.
 * @param promotion_rate The amount that the Rules should be promoted with. If the given amount is 
 * <= 0, nothing will be changed.
 */
void knowledge_base_promote_rules(KnowledgeBase * const knowledge_base,
const RuleQueue * const rules_to_promote, const float promotion_rate) {
    if ((knowledge_base != NULL) && (rules_to_promote != NULL) && promotion_rate > 0) {
        unsigned int i;

        for (i = 0; i < rules_to_promote->length; ++i) {
            int rule_index = rule_queue_find(&(knowledge_base->active),
            &(rules_to_promote->rules[i]));

            if (rule_index != -1) {
                rule_promote(&(knowledge_base->active.rules[rule_index]), promotion_rate);
            } else {
                rule_index = rule_queue_find(&(knowledge_base->inactive),
                &(rules_to_promote->rules[i]));

                if (rule_index != -1) {
                    rule_promote(&(knowledge_base->inactive.rules[rule_index]), promotion_rate);

                    if (knowledge_base->inactive.rules[rule_index].weight >=
                    knowledge_base->activation_threshold) {
                        Rule rule_to_move;

                        rule_queue_remove_rule(&(knowledge_base->inactive), rule_index,
                        &rule_to_move);
                        rule_queue_enqueue(&(knowledge_base->active), &rule_to_move);

                        rule_destruct(&rule_to_move);
                    }
                }
            }
        }
    }
}

/**
 * @brief Demotes all the given Rules in the KnowledgeBase.
 * 
 * @param knowledge_base The KnowledgeBase in which the given Rules should be demoted.
 * @param rules_to_demote The RuleQueue containing the Rules to be demoted.
 * @param demotion_rate The amount that the Rules should be demoted with. If the given amount is 
 * <= 0, nothing will be changed.
 */
void knowledge_base_demote_rules(KnowledgeBase * const knowledge_base,
const RuleQueue * const rules_to_demote, const float demotion_rate) {
    if ((knowledge_base != NULL) && (rules_to_demote != NULL) && (demotion_rate > 0)) {
        unsigned int i;

        for (i = 0; i < rules_to_demote->length; ++i) {
            int rule_index = rule_queue_find(&(knowledge_base->inactive),
            &(rules_to_demote->rules[i]));

            if (rule_index != -1) {
                rule_demote(&(knowledge_base->inactive.rules[rule_index]), demotion_rate);
            } else {
                rule_index = rule_queue_find(&(knowledge_base->active),
                &(rules_to_demote->rules[i]));

                if (rule_index != -1) {
                    rule_demote(&(knowledge_base->active.rules[rule_index]), demotion_rate);

                    if (knowledge_base->active.rules[rule_index].weight <
                    knowledge_base->activation_threshold) {
                        Rule rule_to_move;

                        rule_queue_remove_rule(&(knowledge_base->active),
                        rule_index, &rule_to_move);
                        rule_queue_enqueue(&(knowledge_base->inactive), &rule_to_move);

                        rule_destruct(&rule_to_move);
                    }
                }
            }
        }
    }
}

/**
 * @brief Converts the KnowledgeBase to a string.
 * 
 * @param knowledge_base The KnowledgeBase to be converted.
 * @return The string format of the given KnowledgeBase. Use free() to deallocate this string. Returns 
 * NULL if the KnowledgeBase is NULL.
 */
char *knowledge_base_to_string(const KnowledgeBase * const knowledge_base) {
    char *result, *temp, *rule_queue_string;
    const char * const beginning = "Knowledge Base:\n", * const active_rules = "Active Rules: ",
    * const inactive_rules = "Inactive Rules: ", * const threshold = "Activation Threshold: ";

    size_t result_size = strlen(beginning) + strlen(active_rules) + strlen(inactive_rules) + 
    strlen(threshold) + 1;

    char weight_length[50];
    int weight_size = sprintf(weight_length, "%.4f", knowledge_base->activation_threshold);

    result = strdup(beginning);

    rule_queue_string = rule_queue_to_string(&(knowledge_base->active));
    temp = strdup(result);
    result_size += strlen(rule_queue_string) + weight_size + 2;
    result = (char *) realloc(result, result_size);

    sprintf(result, "%s%s%.4f\n%s%s\n", temp, threshold, knowledge_base->activation_threshold,
    active_rules, rule_queue_string);

    free(temp);
    free(rule_queue_string);

    rule_queue_string = rule_queue_to_string(&(knowledge_base->inactive));
    temp = strdup(result);
    result_size += strlen(rule_queue_string);
    result = (char *) realloc(result, result_size);

    sprintf(result, "%s%s%s", temp, inactive_rules, rule_queue_string);
    
    free(temp);
    free(rule_queue_string);
    return result;
}