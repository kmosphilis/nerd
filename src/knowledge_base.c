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