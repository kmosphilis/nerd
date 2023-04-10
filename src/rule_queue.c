#include <string.h>

#include "rule_queue.h"

typedef struct _RuleQueue {
    RuleQueue rule_queue;
    bool ownership;
} _RuleQueue;

/**
 * @brief Constructs a RuleQueue.
 *
 * @param take_ownership Indicates whether the RuleQueue should take onwership of the Rules that
 * will be added or just keep their reference. If true is given it will take their ownership,
 * otherwise +it will not.
 *
 * @return A new RuleQueue *. Use rule_queue_destructor to deallocate.
 */
RuleQueue *rule_queue_constructor(const bool take_ownership) {
    _RuleQueue *queue = (_RuleQueue *) malloc(sizeof(_RuleQueue));
    queue->rule_queue.length = 0;
    queue->rule_queue.rules = NULL;
    queue->ownership = take_ownership;
    return &(queue->rule_queue);
}

/**
 * @brief Destructs a RuleQueue.
 *
 * @param rule_queue The RuleQueue to be destructed.
 */
void rule_queue_destructor(RuleQueue ** const rule_queue) {
    if (rule_queue && (*rule_queue)) {
        if ((*rule_queue)->rules) {
            unsigned int i;
            if (((_RuleQueue *) *rule_queue)->ownership) {
                for (i = 0; i < (*rule_queue)->length; ++i) {
                    if ((*rule_queue)->rules[i]) {
                        rule_destructor(&((*rule_queue)->rules[i]));
                    }
                }
            }

            free((*rule_queue)->rules);
            (*rule_queue)->rules = NULL;
            (*rule_queue)->length = 0;
        }
        free(*rule_queue);
        *rule_queue = NULL;
    }
}

/**
 * @brief Makes a copy of the given RuleQueue.
 *
 * @param destination The RuleQueue to save the copy. It should be a reference to the struct's
 * pointer (to a RuleQueue *).
 * @param source The RuleQueue to be copied. If the RuleQueue is NULL, the contents of the
 * destination will not be changed.
 */
void rule_queue_copy(RuleQueue ** const destination, const RuleQueue * const source) {
    if (destination && source) {
        _RuleQueue *_source = (_RuleQueue *) source;
        *destination = rule_queue_constructor(_source->ownership);

        if (source->length == 0) {
            (*destination)->rules = NULL;
            (*destination)->length = 0;
            return;
        }

        (*destination)->length = source->length;
        (*destination)->rules = (Rule **) malloc(source->length * sizeof(Rule *));

        unsigned int i;
        if (_source->ownership) {
            for (i = 0; i < source->length; ++i) {
                rule_copy(&((*destination)->rules[i]), source->rules[i]);
            }
        } else {
            for (i = 0; i < source->length; ++i) {
                (*destination)->rules[i] = source->rules[i];
            }
        }
    }
}

/**
 * @brief Shows whether the RuleQueue is taking the ownership of the Rules that will be (have been)
 * added.
 *
 * @param rule_queue The RuleQueue to find if it takes ownership or not.
 *
 * @return 1 (true) if it taking ownership, 0 (false) if it does not or -1 if the rule_queue is
 * NULL.
*/
int rule_queue_is_taking_ownership(const RuleQueue * const rule_queue) {
    if (rule_queue) {
        return ((_RuleQueue *) rule_queue)->ownership;
    }
    return -1;
}

/**
 * @brief Enqueues a Rule to a RuleQueue.
 *
 * @param rule_queue The RuleQueue to enqueue (add) the rule into.
 * @param rule The Rule to be enqueued. It should be reference to a Rule * (Rule ** - a pointer to a
 * Rule *). If the given rule_queue was constructed to take ownership, this parameter will become
 * NULL. If not, it will not become NULL. If NULL is given, the queue will remain the same.
 */
void rule_queue_enqueue(RuleQueue * const rule_queue, Rule ** const rule) {
    if (rule_queue && rule && (*rule)) {
        ++rule_queue->length;
        rule_queue->rules = (Rule **) realloc(rule_queue->rules,
        rule_queue->length * sizeof(Rule *));
        rule_queue->rules[rule_queue->length - 1] = *rule;
        if (((_RuleQueue *) rule_queue)->ownership) {
            *rule = NULL;
        }
    }
}

/**
 * @brief Dequeues a Rule from the RuleQueue.
 *
 * @param rule_queue The RuleQueue to dequeue (remove) the Rule from.
 * @param dequeued_rule A place to save the Rule that will be dequeued. It should be a reference to
 * the struct's pointer (Rule **). Make sure that the given double pointer is not already allocated,
 * otherwise its contents will be lost in the memory. If NULL is given, the Rule will be destroyed.
*/
void rule_queue_dequeue(RuleQueue * const rule_queue, Rule ** const dequeued_rule) {
    if (rule_queue) {
        if (rule_queue->rules) {
            --rule_queue->length;
            if (dequeued_rule) {
                *dequeued_rule = rule_queue->rules[0];
            } else if (((_RuleQueue *) rule_queue)->ownership) {
                rule_destructor(&(rule_queue->rules[0]));
            }

            if (rule_queue->length == 0) {
                free(rule_queue->rules);
                rule_queue->rules = NULL;
            } else {
                Rule **rules = rule_queue->rules;
                rule_queue->rules = (Rule **) malloc(rule_queue->length * sizeof(Rule *));

                memcpy(rule_queue->rules, (rules + 1), rule_queue->length * sizeof(Rule *));

                free(rules);
            }
        }
    }
}

/**
 * @brief Finds the index of the given Rule in the RuleQueue.
 *
 * @param rule_queue The RuleQueue to find the Rule in.
 * @param rule The Rule to be found.
 *
 * @return index where the Rule is or -1 if it does not exist or either the Rule or the RuleQueue
 * are NULL.
 */
int rule_queue_find(const RuleQueue * const  rule_queue, const Rule * const rule) {
    if (rule_queue && rule) {
        unsigned int i;
        for (i = 0; i < rule_queue->length; ++i) {
            if (rule_equals(rule_queue->rules[i], rule)) {
                return i;
            }
        }
        return -1;
    }
    return -2;
}

/**
 * @brief Removes a Rule in the given position.
 *
 * @param rule_queue The RuleQueue to remove the Rule.
 * @param rule_index The position of the Rule to be removed.
 * @param removed_rule A place to save the Rule that will be removed. It should be a reference to
 * the struct's pointer (Rule **). Make sure that the given double pointer is not already allocated,
 * otherwise its contents will be lost in the memory. If NULL is given, the Rule will be destroyed.
*/
void rule_queue_remove_rule(RuleQueue * const rule_queue, const int rule_index,
Rule ** const removed_rule) {
    if (rule_queue && (rule_index >= 0)) {
        const unsigned int u_rule_index = rule_index;
        if (rule_queue->rules && (u_rule_index < rule_queue->length)) {
            --rule_queue->length;
            if (removed_rule) {
                *removed_rule = rule_queue->rules[u_rule_index];
            } else if (((_RuleQueue *) rule_queue)->ownership) {
                rule_destructor(&(rule_queue->rules[u_rule_index]));
            }

            if (rule_queue->length == 0) {
                free(rule_queue->rules);
                rule_queue->rules = NULL;
            } else {
                Rule **rules = rule_queue->rules;

                rule_queue->rules = (Rule **) malloc(rule_queue->length * sizeof(Rule *));

                if (u_rule_index == 0) {
                    memcpy(rule_queue->rules, rules + 1, rule_queue->length * sizeof(Rule *));
                } else if (u_rule_index == (rule_queue->length + 1)) {
                    memcpy(rule_queue->rules, rules, rule_queue->length * sizeof(Rule *));
                } else {
                    memcpy(rule_queue->rules, rules, (u_rule_index) * sizeof(Rule *));
                    memcpy(rule_queue->rules + u_rule_index, rules + u_rule_index + 1,
                    (rule_queue->length - u_rule_index) * sizeof(Rule *));
                }

                free(rules);
            }
        }
    }
}

/**
 * @brief Find all the applicable Rules with a given Context. An applicable Rule, is a Rule whose
 * body is true.
 *
 * @param rule_queue The RuleQueue to find the applicable Rules from.
 * @param context The Context to check for applicable Rules.
 * @param rule_indices The IntVector to save the indices of the Rules that are applicable. It should
 * be a reference to the struct's pointer (to a IntVector *).
 */
void rule_queue_find_applicable_rules(const RuleQueue * const rule_queue,
const Context * const context, IntVector ** const rule_indices) {
    if (rule_queue && context && rule_indices) {
        *rule_indices = int_vector_constructor();

        unsigned int i;
        for (i = 0; i < rule_queue->length; ++i) {
            if (rule_applicable(rule_queue->rules[i], context)) {
                int_vector_push(*rule_indices, i);
            }
        }
    }
}

/**
 * @brief Finds all the concurring rules with a given Context. A concurring Rule, is a Rule whose
 * body and its head are true.
 *
 * @param rule_queue The RuleQueue to find the concurring Rules from.
 * @param context The Context to check for concurring Rules.
 * @param rule_indices The IntVector to save the indices of the Rules that are concurring. It should
 * be a reference to the struct's pointer (IntVector *).
 */
void rule_queue_find_concurring_rules(const RuleQueue * const rule_queue,
const Context * const context, IntVector ** const rule_indices) {
    if (rule_queue && context && rule_indices) {
        *rule_indices = int_vector_constructor();

        unsigned int i, j;
        for (i = 0; i < rule_queue->length; ++i) {
            for (j = 0; j < context->size; ++j) {
                if (literal_equals(rule_queue->rules[i]->head, context->literals[j])) {
                    if (rule_applicable(rule_queue->rules[i], context)) {
                        int_vector_push(*rule_indices, i);
                    }
                    break;
                }
            }
        }
    }
}

/**
 * @brief Converts the RuleQueue into a string format.
 *
 * @param rule_queue The RuleQueue to be converted.
 *
 * @return The string format of the given RuleQueue. Use free() to deallocate this string. Returns
 * NULL if the RuleQueue is NULL.
 */
char *rule_queue_to_string(const RuleQueue * const rule_queue) {
    if (rule_queue) {
        if (rule_queue->length == 0) {
            return strdup("[\n]");
        }
        char *result, *temp, *beginning = "[\n";

        size_t result_size = strlen(beginning) + 1;
        result = strdup(beginning);
        char *rule_string;
        unsigned int i;

        for (i = 0; i < rule_queue->length - 1; ++i) {
            rule_string = rule_to_string(rule_queue->rules[i]);
            result_size += strlen(rule_string) + 3;
            temp = strdup(result);
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s\t%s,\n", temp, rule_string);
            free(rule_string);
            free(temp);
        }

        rule_string = rule_to_string(rule_queue->rules[rule_queue->length - 1]);
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
