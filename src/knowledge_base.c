#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pcg_variants.h>

#include "nerd_utils.h"
#include "knowledge_base.h"

/**
 * @brief Constructs a KnowledgeBase.
 *
 * @param activation_threshold The threshold which determines whether a rule should get activated or
 * deactivated.
 * @param use_backward_chaining A boolean value which indicates whether the hypergraph should demoted
 * rules using the backward chaining algorithm or not.
 *
 * @return A new KnowledgeBase object *. Use knowledge_base_destructor to deallocate.
 */
KnowledgeBase *knowledge_base_constructor(const float activation_threshold,
const bool use_backward_chaining) {
    KnowledgeBase *knowledge_base = (KnowledgeBase *) malloc(sizeof(KnowledgeBase));
    knowledge_base->activation_threshold = activation_threshold;
    knowledge_base->active = rule_queue_constructor(false);
    knowledge_base->hypergraph = rule_hypergraph_empty_constructor(use_backward_chaining);
    return knowledge_base;
}

/**
 * @brief Destructs a KnowledgeBase.
 *
 * @param knowledge_base The KnowledgeBase to be destructed. It should be a reference to the
 * object's pointer. If NULL the process will fail.
 */
void knowledge_base_destructor(KnowledgeBase ** const knowledge_base) {
    if (knowledge_base && (*knowledge_base)) {
        rule_queue_destructor(&((*knowledge_base)->active));
        rule_hypergraph_destructor(&((*knowledge_base)->hypergraph));
        (*knowledge_base)->activation_threshold = INFINITY;
        safe_free(*knowledge_base);
    }
}

/**
 * @brief Makes a copy of the given KnowledgeBase.
 *
 * @param destination The KnowledgeBase to save the copy. It should be a reference to the object's
 * pointer.
 * @param source The KnowledgeBase to be copied. If the KnowledgeBase is NULL, the contents of the
 * destination will not be changed.
 */
void knowledge_base_copy(KnowledgeBase ** const restrict destination,
const KnowledgeBase * const restrict source) {
    if (destination && source) {
        *destination = (KnowledgeBase *) malloc(sizeof(KnowledgeBase));
        (*destination)->activation_threshold = source->activation_threshold;
        (*destination)->active = rule_queue_constructor(false);
        rule_hypergraph_copy(destination, source);
    }
}

/**
 * @brief Adds a Rule in the KnowledgeBase by taking its ownership. If the weight of the Rule is
 * above the activation_threshold, the Rule will be added to the active RuleQueue, otherwise it will
 *  be added to the inactive RuleQueue.
 *
 * @param knowledge_base The KnowledgeBase to be expanded.
 * @param rule The Rule to be added. It should be reference to a Rule * (Rule ** - a pointer to a
 * Rule *). If the given Rule had took ownership of its content, a new Rule will be created without
 * taking the ownership of that content as it will belong to the internal RB-Tree. If it does not
 * take the ownership of the its content, the content pointers might stil change, as the body and
 * head will take the pointer of the item allocated in the RB-Tree.
 *
 * @return 1 if the Rule was added successfully, 0 if it was not added, and -1 if one of the
 * parameters is NULL.
 */
int knowledge_base_add_rule(KnowledgeBase * const knowledge_base, Rule ** const rule) {
    if (knowledge_base && rule && (*rule)) {
        if (rule_hypergraph_add_rule(knowledge_base->hypergraph, rule) == 1) {
            if ((*rule)->weight >= knowledge_base->activation_threshold) {
                rule_queue_enqueue(knowledge_base->active, rule);
            }
            return 1;
        }
        return 0;
    }
    return -1;
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
 * @param focused_labels (Optional) A Context containing labels that the algorithm should use as the
 * head for every Rule that it will create. If NULL, the head will be a random uncovered Literal.
 */
void knowledge_base_create_new_rules(KnowledgeBase * const knowledge_base,
const Scene * const restrict observed, const Scene * const restrict inferred,
const unsigned int max_body_size, const unsigned int max_number_of_rules,
const Context * const restrict focused_labels) {
    if (knowledge_base && observed) {
        Context *uncovered = NULL;
        Scene *combined = NULL;
        Literal *head = NULL, *temp = NULL;
        Body *body = NULL;
        Rule *new_rule = NULL;
        bool head_set = false;

        scene_union(observed, inferred, &combined);

        unsigned int i;
        if (focused_labels) {
            for (i = 0; i < focused_labels->size; ++i) {
                int index = scene_literal_index(combined, focused_labels->literals[i]);
                if (index > -1) {
                    scene_remove_literal(combined, index, &head);
                    head_set = true;
                    break;
                }
            }
            if (!head_set) {
                goto failed;
            }

            if (scene_literal_index(inferred, head) > -1) {
                goto failed;
            }
        } else {
            scene_difference(observed, inferred, &uncovered);
        }

        pcg32_random_t *rng = global_rng;
        if (!rng) {
            rng = (pcg32_random_t *) malloc(sizeof(pcg32_random_t));
            pcg32_srandom_r(rng, time(NULL), 42u);
        }

        unsigned int j, body_size,
        rules_to_create = (pcg32_random_r(rng) % max_number_of_rules) + 1;
        int chosen_head_index, head_index, remaining_randoms, random_chosen,chosen_index,
        *random_indices;

        for (i = 0; i < rules_to_create; ++i) {
            if (combined->size > 1) {

                if (!head_set) {
                    if (uncovered->size == 0) {
                        break;
                    }

                    chosen_head_index = pcg32_random_r(rng) % uncovered->size;
                    literal_copy(&head, uncovered->literals[chosen_head_index]);

                    head_index = scene_literal_index(combined, head);
                    if (head_index >= 0) {
                        scene_remove_literal(combined, head_index, NULL);
                    }
                }

                body = context_constructor(true);
                body_size = (pcg32_random_r(rng) % max_body_size) + 1;
                remaining_randoms = combined->size;

                random_indices = (int *) malloc(combined->size * sizeof(int));

                for (j = 0; j < combined->size; ++j) {
                    random_indices[j] = j;
                }

                if (body_size > combined->size) {
                    body_size = combined->size;
                }

                for (j = 0; j < body_size; ++j) {
                    random_chosen = pcg32_random_r(rng) % remaining_randoms;
                    chosen_index = random_indices[random_chosen];
                    random_indices[random_chosen] = random_indices[remaining_randoms - 1];
                    remaining_randoms--;
                    literal_copy(&temp, combined->literals[chosen_index]);
                    context_add_literal(body, &temp);
                }

                literal_copy(&temp, head);
                new_rule = rule_constructor(body->size, body->literals, &temp, 0, true);

                if (knowledge_base_add_rule(knowledge_base, &new_rule) != 1) {
                    rule_destructor(&new_rule);
                }

                if (!head_set) {
                    scene_add_literal(combined, &head);
                }

                safe_free(random_indices);
                context_destructor(&body);
            }
        }

        if (head_set) {
            literal_destructor(&head);
        }

        if (!global_rng) {
            free(rng);
        }

failed:
        literal_destructor(&head);
        scene_destructor(&combined);
        context_destructor(&uncovered);
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
    if (knowledge_base && knowledge_base->active && knowledge_base->hypergraph) {
        char *result, *temp, *rule_queue_string;
        const char * const beginning = "Knowledge Base:\n", * const active_string = "Active Rules: ",
        * const inactive_string = "Inactive Rules: ", * const threshold = "Activation Threshold: ";

        size_t result_size = strlen(beginning) + strlen(active_string) + strlen(inactive_string) +
        strlen(threshold) + 1;

        char weight_length[50];
        int weight_size = sprintf(weight_length, "%.4f", knowledge_base->activation_threshold);

        result = strdup(beginning);

        rule_queue_string = rule_queue_to_string(knowledge_base->active);
        temp = strdup(result);
        result_size += strlen(rule_queue_string) + weight_size + 2;
        result = (char *) realloc(result, result_size);

        sprintf(result, "%s%s%.4f\n%s%s\n", temp, threshold, knowledge_base->activation_threshold,
        active_string, rule_queue_string);

        free(temp);
        free(rule_queue_string);

        RuleQueue *inactive;
        rule_hypergraph_get_inactive_rules(knowledge_base, &inactive);

        rule_queue_string = rule_queue_to_string(inactive);
        rule_queue_destructor(&inactive);
        temp = strdup(result);
        result_size += strlen(rule_queue_string);
        result = (char *) realloc(result, result_size);

        sprintf(result, "%s%s%s", temp, inactive_string, rule_queue_string);

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
    if (knowledge_base && knowledge_base->active && knowledge_base->hypergraph) {
        if (knowledge_base->active->rules) {
            char const *end = "], \"code\": \"\", \"imports\": \"\", \"warnings\": [], "
            "\"customPriorities\": []}";
            char *result = strdup("{\"type\": \"output\", \"kb\": ["), *temp,
            *rule_prudensjs_string;
            size_t result_size = strlen(result) + 1;

            unsigned int i;
            for (i = knowledge_base->active->length; i > 0; --i) {
                rule_prudensjs_string =
                rule_to_prudensjs(knowledge_base->active->rules[i - 1], i - 1);
                result_size += strlen(rule_prudensjs_string) + 2;
                temp = strdup(result);
                result = (char *) realloc(result, result_size);
                sprintf(result, "%s%s, ", temp, rule_prudensjs_string);
                free(rule_prudensjs_string);
                free(temp);
            }

            memset(result + result_size - 3, '\0', 2);
            result_size += strlen(end) - 2;

            temp = strdup(result);

            result = (char *) realloc(result, result_size * sizeof(char));
            sprintf(result, "%s%s", temp, end);
            free(temp);

            return result;
        }
        return strdup("{\"type\": \"output\", \"kb\": [], \"code\": \"\", \"imports\": \"\", "
            "\"warnings\": [], \"customPriorities\": []}");
    }
    return NULL;
}
