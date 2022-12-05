#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "knowledge_base.h"

/**
 * @brief Constructs a KnowledgeBase.
 * 
 * @param knowledge_base The KnowledgeBase to be constructed. If NULL the process will fail.
 * @param activation_threshold The threshold which determines whether a rule should get activated or 
 * deactivated.
 */
void knowledge_base_constructor(KnowledgeBase * const knowledge_base,
const float activation_threshold) {
    if (knowledge_base) {
        knowledge_base->activation_threshold = activation_threshold;
        rule_queue_constructor(&(knowledge_base->active));
        rule_queue_constructor(&(knowledge_base->inactive));
    }
}

/**
 * @brief Destructs a KnowledgeBase.
 * 
 * @param knowledge_base The KnowledgeBase to be destructed. If NULL the process will fail.
 */
void knowledge_base_destructor(KnowledgeBase * const knowledge_base) {
    if (knowledge_base) {
        rule_queue_destructor(&(knowledge_base->active));
        rule_queue_destructor(&(knowledge_base->inactive));
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
void knowledge_base_copy(KnowledgeBase * const restrict destination,
const KnowledgeBase * const restrict source) {
    if (destination && source) {
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
    if (knowledge_base && rule) {
        if (rule->weight >= knowledge_base->activation_threshold) {
            rule_queue_enqueue(&(knowledge_base->active), rule);
        } else {
            rule_queue_enqueue(&(knowledge_base->inactive), rule);
        }
    }
}

/**
 * @brief Creates new Rules by finding uncovered Literals. Uncovered Literals, are Literals that 
 * have been observed, but have not been inferred.
 * 
 * @param knowledge_base The KnowledgeBase to be expanded.
 * @param observed The observed Scene.
 * @param inferred The inferred Scene.
 * @param max_body_size The maximum number of Literals to be included in the body of a Rule.
 * @param max_number_of_rules The maximum number of Rules to be created.
 */
void knowledge_base_create_new_rules(KnowledgeBase * const knowledge_base,
const Scene * const restrict observed, const Scene * const restrict inferred,
const unsigned int max_body_size, const unsigned int max_number_of_rules) {
    if (knowledge_base && observed) {
        Context uncovered;
        Scene combined;

        context_constructor(&uncovered);
        scene_constructor(&combined);

        scene_difference(observed, inferred, &uncovered);
        scene_union(observed, inferred, &combined);

        srand(time(NULL));

        unsigned int i, j, body_size, rules_to_create = (rand() % max_number_of_rules) + 1;

        for (i = 0; i < rules_to_create; ++i) {
            if ((uncovered.size != 0) && (combined.size > 1)) {
                Literal head;
                Body body;
                Rule new_rule;

                srand(time(NULL) + i);
                context_constructor(&body);
                int chosen_head_index = rand() % uncovered.size;
                literal_copy(&head, &(uncovered.observations[chosen_head_index]));

                int head_index = scene_literal_index(&combined, &head);
                if (head_index >= 0) {
                    scene_remove_literal(&combined, head_index);
                }

                body_size = (rand() % max_body_size) + 1;
                int remaining_randoms = combined.size, random_chosen, chosen_index;

                int *random_indices = (int *) calloc(combined.size, sizeof(int));

                for (j = 0; j < combined.size; ++j) {
                    random_indices[j] = j;
                }

                if (body_size > combined.size) {
                    body_size = combined.size;
                }

                for (j = 0; j < body_size; ++j) {
                    random_chosen = rand() % remaining_randoms;
                    chosen_index = random_indices[random_chosen];
                    random_indices[random_chosen] = random_indices[remaining_randoms - 1];
                    remaining_randoms--;
                    context_add_literal(&body, &(combined.observations[chosen_index]));
                }

                rule_constructor(&new_rule, body.size, &(body.observations), &head, 0);

                if (rule_queue_find(&(knowledge_base->active), &new_rule) == -1) {
                    if (rule_queue_find(&(knowledge_base->inactive), &new_rule) == -1) {
                        knowledge_base_add_rule(knowledge_base, &new_rule);
                    }
                }

                scene_add_literal(&combined, &head);
                free(random_indices);
                rule_destructor(&new_rule);
                context_destructor(&body);
                literal_destructor(&head);
            }
        }

        scene_destructor(&combined);
        context_destructor(&uncovered);
    }
}

/**
 * @brief Finds the applicable rules from the given Context/Scene (Literals). If any given parameter
 *  is NULL, the function will not be executed. An applicable Rule, is a Rule whose body is true. 
 * The head can be true or false.
 * 
 * @param knowledge_base The KnowledgeBase to find the Rules from.
 * @param context The context Context (Scene).
 * @param applicable_active_rules The IntVector to save the active applicable Rules.
 * @param applicable_inactive_rules The IntVector to save the inactive applicable Rules.
 */
void knowledge_base_applicable_rules(const KnowledgeBase * const knowledge_base,
const Context * const context, IntVector * const restrict applicable_active_rules,
IntVector * const restrict applicable_inactive_rules) {
    if (knowledge_base && context && applicable_active_rules && applicable_inactive_rules) {
        rule_queue_find_applicable_rules(&(knowledge_base->active), context,
        applicable_active_rules);
        rule_queue_find_applicable_rules(&(knowledge_base->inactive), context,
        applicable_inactive_rules);
    }
}

//TODO Rename the function to effective rules, and create a new one in which concurring rules, are 
// rules whose head is true and bodu can be false or true.
/**
 * @brief Finds the concurring Rules from the given Context/Scene (Literals). If any given parameter
 *  is NULL, the function will not be executed. A concurring Rule, is a Rule whose body and head are
 *  true.
 * 
 * @param knowledge_base The KnowledgeBase to find the Rules from.
 * @param context The context Context (Scene).
 * @param concurring_active_rules The IntVector to save the active concurring Rules.
 * @param concurring_inactive_rules The IntVector to save the inactive concurring Rules.
 */
void knowledge_base_concurring_rules(const KnowledgeBase * const knowledge_base,
const Context * const context, IntVector * const restrict concurring_active_rules,
IntVector * const restrict concurring_inactive_rules) {
    if (knowledge_base && context && concurring_active_rules && concurring_inactive_rules) {
        rule_queue_find_concurring_rules(&(knowledge_base->active), context,
        concurring_active_rules);
        rule_queue_find_concurring_rules(&(knowledge_base->inactive), context,
        concurring_inactive_rules);
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
    if (knowledge_base && rules_to_promote && (promotion_rate > 0)) {
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

                        rule_destructor(&rule_to_move);
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
    if (knowledge_base && rules_to_demote && (demotion_rate > 0)) {
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

                        rule_destructor(&rule_to_move);
                    }
                }
            }
        }
    }
}

/**
 * @brief Promotes a Rule in the given KnowledgeBase. If the Rule is inactive and its weight is 
 * higher or equal to the activation_threshold, then the Rule will become active.
 * 
 * @param knowledge_base The KnowledgeBase in which the the given Rule should be promoted.
 * @param type The type of the Rule to be promoted.
 * @param rule_index The index of the rule to be promoted. If the index is not within bounds, the 
 * function will be ignored.
 * @param promotion_weight The amount that the Rule should be promoted with. If the amount is <= 0,
 * nothing will be changed.
 * @return 1 if Rule was INACTIVE and it was promoted to ACTIVE, 0 if a Rule was simply promoted, 
 * and -1 if the given KnowledgeBase is NULL.
 */
int knowledge_base_promote_rule(KnowledgeBase * const knowledge_base, const RuleType type,
const unsigned int rule_index, const float promotion_weight) {
    if (knowledge_base && (promotion_weight > 0)) {
        if ((type == ACTIVE) && (knowledge_base->active.length > rule_index)) {
            rule_promote(&(knowledge_base->active.rules[rule_index]), promotion_weight);
        } else if ((type == INACTIVE) && (knowledge_base->inactive.length > rule_index)) {
            rule_promote(&(knowledge_base->inactive.rules[rule_index]), promotion_weight);

            if (knowledge_base->inactive.rules[rule_index].weight >=
            knowledge_base->activation_threshold) {
                Rule rule_to_move;

                rule_queue_remove_rule(&(knowledge_base->inactive), rule_index, &rule_to_move);
                rule_queue_enqueue(&(knowledge_base->active), &rule_to_move);

                rule_destructor(&rule_to_move);
                return 1;
            }
        }
        return 0;
    }
    return -1;
}

/**
 * @brief Demotes a rule in the given KnowledgeBase. If the Rule is ACTIVE and its weight is lower
 * than the activation_threshold, then the Rule will become INACTIVE. If the Rule is INACTIVE and
 * its weight is 0, then it will get deleted.
 *
 * @param knowledge_base The KnowledgeBase in which the the given Rule should be demoted.
 * @param type The type of the Rule to be demoted.
 * @param rule_index The index of the rule to be demoted. If the index is not within bounds, the
 * function will be ignored.
 * @param demotion_weight The amount that the Rule should be demoted with. If the amount is <= 0,
 * nothing will be changed.
 * @return 1 if Rule was ACTIVE and it was demoted to INACTIVE, 2 if the Rule was INACTIVE and was
 * deleted, 0 if a Rule was simply demoted, and -1 if the given KnowledgeBase is NULL.
 */
int knowledge_base_demote_rule(KnowledgeBase * const knowledge_base, const RuleType type,
const unsigned int rule_index, const float demotion_weight) {
    if (knowledge_base && (demotion_weight > 0)) {
        if ((type == ACTIVE) && (knowledge_base->active.length > rule_index)) {
            rule_demote(&(knowledge_base->active.rules[rule_index]), demotion_weight);

            if (knowledge_base->active.rules[rule_index].weight <
            knowledge_base->activation_threshold) {
                Rule rule_to_move;

                rule_queue_remove_rule(&(knowledge_base->active), rule_index, &rule_to_move);
                rule_queue_enqueue(&(knowledge_base->inactive), &rule_to_move);

                rule_destructor(&rule_to_move);
                return 1;
            }
        } else if ((type == INACTIVE) && (knowledge_base->inactive.length > rule_index)) {
            if (knowledge_base->inactive.rules[rule_index].weight == 0) {
                rule_queue_remove_rule(&(knowledge_base->inactive), rule_index, NULL);
                return 2;
            }
            rule_demote(&(knowledge_base->inactive.rules[rule_index]), demotion_weight);
        }
        return 0;
    }
    return -1;
}


// TODO Required re-thinking using a better implementation.
/**
 * @brief knowledge_base_demote_chained_rules helper.
 */ 
void _knowledge_base_demote_chained_rules(KnowledgeBase * const knowledge_base,
const Scene * const inferred, IntVector * const applicable_rules, const RuleType type,
const unsigned int rule_to_demote, const float demotion_weight,
float ** const applicable_rules_demotions,
int recursion_number) {
    if (!(knowledge_base || inferred || applicable_rules || (demotion_weight > 0))) {
        return;
    }

    Context inferred_body_literals;
    unsigned int i, j;

    context_constructor(&inferred_body_literals);

    switch (type) {
        case ACTIVE:
            scene_intersect(inferred, &(knowledge_base->active.rules[rule_to_demote].body),
            &inferred_body_literals);
            break;
        case INACTIVE:
            scene_intersect(inferred, &(knowledge_base->inactive.rules[rule_to_demote].body),
            &inferred_body_literals);
            break;
        default:
            return;
    }

    for (i = 0; i < applicable_rules->size; ++i) {
        unsigned int rule_index = int_vector_get(applicable_rules, i);
        Rule *rule = &(knowledge_base->active.rules[rule_index]);
        if (rule_concurs(rule, &inferred_body_literals)) {
            *(applicable_rules_demotions[i]) += demotion_weight * (1/recursion_number);

            if (applicable_rules->size == 1) {
                continue;
            }

            IntVector new;
            int_vector_copy(&new, applicable_rules);
            int_vector_delete(&new, i);
            float **remaining_applicable_demotions = (float **) malloc(sizeof(float *) * new.size);

            for (j = 0; j < i; ++j) {
                remaining_applicable_demotions[j] = applicable_rules_demotions[j];
            }

            for (j = i + 1; j < applicable_rules->size; ++j) {
                remaining_applicable_demotions[j - 1] = applicable_rules_demotions[j];
            }

            _knowledge_base_demote_chained_rules(knowledge_base, inferred, &new, ACTIVE,
            rule_index, demotion_weight, remaining_applicable_demotions,
            recursion_number + 1);
            free(remaining_applicable_demotions);
            int_vector_destructor(&new);
        }
    }

    scene_destructor(&inferred_body_literals);
}

/**
 * @brief Demotes a rule and all the rules that inferred its body (if any exist) in the given
 * KnowledgeBase. If the Rule is ACTIVE and its weight is lower than the activation_threshold, then
 * the Rule will become INACTIVE. If the Rule is INACTIVE and its weight is 0, then it will get
 * deleted. If any parameter is NULL, the function will exit.
 *
 * @param knowledge_base The KnowledgeBase in which the the given Rule should be demoted.
 * @param inferred The inferred Scene.
 * @param applicable_rules An IntVector containing all the applicable rules. If any of these rules
 * gets demoted or deleted during the process, it will removed from the list.
 * @param type The type of the Rule to be demoted.
 * @param rule_to_demote The index of the rule to be demoted. If the index is not within bounds, the
 * function will be ignored.
 * @param demotion_weight The amount that the Rule should be demoted with. If the amount is <= 0,
 * nothing will be changed.
 * @param demoted_applicable_rules An IntVector to save all the Rules that were demoted from
 * applicable_rules.
 * @return 1 if Rule was ACTIVE and it was demoted to INACTIVE, 2 if the Rule was INACTIVE and was
 * deleted, 0 if a Rule was simply demoted, and -1 if any of the parameters is NULL.
 */
int knowledge_base_demote_chained_rules(KnowledgeBase * const knowledge_base,
const Scene * const inferred, IntVector * const applicable_rules, const RuleType type,
const unsigned int rule_to_demote, const float demotion_weight,
IntVector * const demoted_applicable_rules) {
    if (!(knowledge_base && inferred && applicable_rules)) {
        return -1;
    }

    float **applicable_rules_demotion = (float **) malloc(sizeof(float *) * applicable_rules->size);
    unsigned int active_rules_removed = 0;
    int i;
    unsigned int j;
    IntVector deleted_applicables_rules;

    int_vector_constructor(&deleted_applicables_rules);

    for (j = 0; j < applicable_rules->size; ++j) {
        applicable_rules_demotion[j] = (float *) malloc(sizeof(float));
        *(applicable_rules_demotion[j]) = 0;
    }

    _knowledge_base_demote_chained_rules(knowledge_base, inferred, applicable_rules, type,
    rule_to_demote, demotion_weight, applicable_rules_demotion, 1);

    if (type == ACTIVE) {
        for (i = applicable_rules->size - 1; i >= 0; --i) {
            unsigned int rule_index = int_vector_get(applicable_rules, i);
            if (knowledge_base->active.rules[rule_index].weight <
            *(applicable_rules_demotion[i])) {
                if (rule_index < rule_to_demote) { 
                    ++active_rules_removed;
                }
                int_vector_delete(applicable_rules, i);
                rule_queue_remove_rule(&(knowledge_base->active), rule_index, NULL);
                for (j = 0; j < (unsigned int) i; ++j) {
                    --applicable_rules->items[j];
                }
            } else if (knowledge_base_demote_rule(knowledge_base, ACTIVE, rule_index,
            *(applicable_rules_demotion[i])) == 1) {
                if (rule_index < rule_to_demote) {
                    ++active_rules_removed;
                }
                int_vector_delete(applicable_rules, i);
                if (demoted_applicable_rules) {
                    int_vector_push(demoted_applicable_rules, rule_index);
                }
                for (j = 0; j < (unsigned int) i; ++j) {
                    --applicable_rules->items[j];
                }
            }
            free(applicable_rules_demotion[i]);
        }
        free(applicable_rules_demotion);
    } else {
        for (i = applicable_rules->size - 1; i >= 0; --i) {
            unsigned int rule_index = int_vector_get(applicable_rules, i);
            if (knowledge_base->active.rules[rule_index].weight <
            *(applicable_rules_demotion[i])) {
                int_vector_delete(applicable_rules, i);
                rule_queue_remove_rule(&(knowledge_base->active), rule_index, NULL);
                for (j = 0; j < (unsigned int) i; ++j) {
                    --applicable_rules->items[j];
                }
            } else if (knowledge_base_demote_rule(knowledge_base, ACTIVE, rule_index,
            *(applicable_rules_demotion[i])) == 1) {
                int_vector_delete(applicable_rules, i);
                if (demoted_applicable_rules) {
                    int_vector_push(demoted_applicable_rules, rule_index);
                }
                for (j = 0; j < (unsigned int) i; ++j) {
                    --applicable_rules->items[j];
                }
            }
            free(applicable_rules_demotion[i]);
        }
        free(applicable_rules_demotion);
    }

    switch (type) {
        case ACTIVE:
            switch (knowledge_base_demote_rule(knowledge_base, ACTIVE,
            rule_to_demote - active_rules_removed, demotion_weight)) {
                case 1:
                    for (j = 0; j < applicable_rules->size; ++j) {
                        if ((unsigned int) int_vector_get(applicable_rules, j) == 
                        (rule_to_demote - active_rules_removed)) {
                            int_vector_delete(applicable_rules, j);

                            unsigned int k;
                            for (k = 0; k < j; ++k) {
                                --applicable_rules->items[k];
                            }
                            
                            return 1;
                        }
                    }
                    return 1;
                default:
                    return 0;
            }
        default:
            return knowledge_base_demote_rule(knowledge_base, INACTIVE, rule_to_demote,
            demotion_weight);
    }
}

/**
 * @brief Converts the KnowledgeBase to a string.
 * 
 * @param knowledge_base The KnowledgeBase to be converted.
 * @return The string format of the given KnowledgeBase. Use free() to deallocate this string. 
 * Returns NULL if the KnowledgeBase is NULL.
 */
char *knowledge_base_to_string(const KnowledgeBase * const knowledge_base) {
    if (knowledge_base) {
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
    return NULL;
}

/**
 * @brief Converts a KnowledgeBase to a Prudens JS Knowledge base format. Note: It only returns the 
 * active rules of a KnowledgeBase, since the inactive should not be considered until their 
 * activation.
 * 
 * @param knowledge_base The KnowledgeBase to be converted.
 * @return The Prudens JS Knowledge base format (as a string) of the given KnowledgeBase. The rules 
 * will be in reverse order since younger rules in Prudens JS have higher priority unlike NERD, 
 * where older rules have higher priority. Use free() to deallocate the result. Returns NULL if the 
 * given KnowledgeBase * is NULL.
 */
char *knowledge_base_to_prudensjs(const KnowledgeBase * const knowledge_base) {
    if (knowledge_base) {
        if (knowledge_base->active.rules) {
            char const *end = "], \"code\": \"\", \"imports\": \"\", "
            "\"warnings\": [], \"customPriorities\": []}";
            char *result = strdup("{\"type\": \"output\", \"kb\": ["), *temp,
            *rule_prudensjs_string;
            size_t result_size = strlen(result) + 1;

            unsigned int i;
            for (i = knowledge_base->active.length - 1; i > 0; --i) {
                rule_prudensjs_string = rule_to_prudensjs(&(knowledge_base->active.rules[i]), i);
                result_size += strlen(rule_prudensjs_string) + 2;
                temp = strdup(result);
                result = (char *) realloc(result, result_size);
                sprintf(result, "%s%s, ", temp, rule_prudensjs_string);
                free(rule_prudensjs_string);
                free(temp);
            }

            rule_prudensjs_string = rule_to_prudensjs(&(knowledge_base->active.rules[i]), i);
            result_size += strlen(rule_prudensjs_string) + strlen(end);
            temp = strdup(result);
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s%s%s", temp, rule_prudensjs_string, end);
            free(rule_prudensjs_string);
            free(temp);

            return result;
        }
        return strdup("{\"type\": \"output\", \"kb\": [], \"code\": \"\", \"imports\": \"\", "
            "\"warnings\": [], \"customPriorities\": []}");
    }
    return NULL;
}