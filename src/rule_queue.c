#include <malloc.h>
#include <string.h>

#include "rule_queue.h"

/**
 * @brief Constructs a RuleQueue.
 * 
 * @param rule_queue The RuleQueue to be constructed. If NULL is given, a queue will not be 
 * constructed.
 */
void rule_queue_construct(RuleQueue * const rule_queue) {
    if (rule_queue != NULL) {
        rule_queue->length = 0;
        rule_queue->rules = NULL;
    }
}

/**
 * @brief Destructs a RuleQueue.
 * 
 * @param rule_queue The RuleQueue to be destructed.
 */
void rule_queue_destruct(RuleQueue * const rule_queue) {
    if (rule_queue != NULL) {
        if (rule_queue->rules != NULL) {
            unsigned int i;
            for (i = 0; i < rule_queue->length; ++i) {
                rule_destruct(&(rule_queue->rules[i]));
            }

            free(rule_queue->rules);
            rule_queue->rules = NULL;
            rule_queue->length = 0;
        }
    }
}

/**
 * @brief Makes a copy of the given RuleQueue.
 * 
 * @param destination The RuleQueue to save the copy.
 * @param source The RuleQueue to be copied. If the RuleQueue is NULL, the contents of the 
 * destination will not be changed.
 */
void rule_queue_copy(RuleQueue * const destination, const RuleQueue * const source) {
    if ((destination != NULL) && (source != NULL)) {
        destination->length = source->length;
        destination->rules = (Rule *) malloc(source->length * sizeof(Rule));

        unsigned int i;
        for (i = 0; i < source->length; ++i) {
            rule_copy(&(destination->rules[i]), &(source->rules[i]));
        }
    }
}

/**
 * @brief Enqueues a Rule to a RuleQueue.
 * 
 * @param rule_queue The RuleQueue to enqueue (add) the rule into.
 * @param rule The Rule to be enqueued. If NULL is given, the queue will remain the same.
 */
void rule_queue_enqueue(RuleQueue * const rule_queue, const Rule * const rule) {
    if ((rule_queue != NULL) && (rule != NULL)) {
        ++rule_queue->length;
        rule_queue->rules = (Rule *) realloc(rule_queue->rules, rule_queue->length * sizeof(Rule));
        rule_copy(&(rule_queue->rules[rule_queue->length - 1]), rule);
    }
}

/**
 * @brief Dequeues a Rule from the RuleQueue.
 * 
 * @param rule_queue The RuleQueue to dequeue (remove) the rule from.
 * @param dequeued_rule The Rule that was dequeued to be saved. If NULL is given, the rule will be 
 * destroyed.
 */
void rule_queue_dequeue(RuleQueue * const rule_queue, Rule * dequeued_rule) {
    if (rule_queue != NULL) {
        if (rule_queue->rules != NULL) {
            --rule_queue->length;
            if (dequeued_rule != NULL) {
                Rule *rule = &(rule_queue->rules[0]);
                dequeued_rule->body = rule->body;
                dequeued_rule->body_size = rule->body_size;
                dequeued_rule->head = rule->head;
                dequeued_rule->weight = rule->weight;
            } else {
                rule_destruct(&(rule_queue->rules[0]));
            }

            if (rule_queue->length == 0) {
                free(rule_queue->rules);
                rule_queue->rules = NULL;
            } else {
                Rule *rules = rule_queue->rules;
                rule_queue->rules = (Rule *) malloc(rule_queue->length * sizeof(Rule));

                memcpy(rule_queue->rules, (rules + 1), rule_queue->length * sizeof(Rule));

                free(rules);
            }
        }
    }
}

/**
 * @brief Converts the RuleQueue into a string format.
 * 
 * @param rule_queue The RuleQueue to be converted.
 * @return The string format of the given RuleQueue. Use free() to deallocate this string. Returns 
 * NULL if the RuleQueue is NULL.
 */
char *rule_queue_to_string(const RuleQueue * const rule_queue) {
    if (rule_queue != NULL) {
        if (rule_queue->length == 0) {
            return strdup("[\n]");
        }
        char *result, *temp, *beginning = "[\n";

        size_t result_size = strlen(beginning) + 1;
        result = strdup(beginning);
        char *rule_string;
        unsigned int i;

        for (i = 0; i < rule_queue->length - 1; ++i) {
            rule_string = rule_to_string(&(rule_queue->rules[i]));
            result_size += strlen(rule_string) + 3;
            temp = strdup(result);
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s\t%s,\n", temp, rule_string);
            free(rule_string);
            free(temp);
        }

        rule_string = rule_to_string(&(rule_queue->rules[rule_queue->length - 1]));
        result_size += strlen(rule_string) + 3;
        temp = strdup(result);
        result = (char *) realloc(result, result_size);
        sprintf(result, "%s\t%s\n]", temp, rule_string);
        free(rule_string);
        free(temp);

        return result;
    }
    return NULL;
}