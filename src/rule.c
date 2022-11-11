#include <malloc.h>
#include <math.h>
#include <string.h>

#include "rule.h"

/**
 * @brief Constructs a Rule. If the body is NULL, the body_size is > 0 or the head's atom is empty, 
 * the Rule that will be created will be in a destructed, unusable form.
 * 
 * @param rule The Rule to be constructed.
 * @param body_size The size of the body of the rule.
 * @param body An array containing a series of Literals required to activate the rule.
 * @param head The head when the rule gets activated.
 * @param weight The weight of the rule.
 */
void rule_constructor(Rule * const rule, const unsigned int body_size, Literal ** const body,
const Literal * const head, const float weight) {
    if (rule != NULL) {
        if ((head != NULL) && (body != NULL) && (body_size > 0)) {
            literal_copy(&(rule->head), head);

            rule->body_size = body_size;

            rule->body = (Literal *) malloc(body_size * sizeof(Literal));

            unsigned int i;
            for (i = 0; i < body_size; ++i) {
                literal_copy(&(rule->body[i]), &(*body)[i]);
            }
            rule->weight = weight;
        } else {
            rule->body_size = 0;
            literal_destructor(&(rule->head));
            rule->body = NULL;
            rule->weight = INFINITY;
        }
    }
}

/**
 * @brief Deconstructs a Rule.
 * 
 * @param rule The Rule to be destructed.
 */
void rule_destructor(Rule * const rule) {
    if (rule != NULL) {
        literal_destructor(&(rule->head));
        if (rule->body != NULL) {
            unsigned int i;
            for (i = 0; i < rule->body_size; ++i) {
                literal_destructor(&(rule->body[i]));
            }
            free(rule->body);
            rule->body = NULL;
        }
        rule->body_size = 0;
        rule->weight = INFINITY;
    }
}

/**
 * @brief Makes a copy of the given Rule.
 * 
 * @param destination The Rule to save the copy.
 * @param source The Rule to be copied. If the Rule, its body or the head's atom are NULL, the 
 * content of the destination will not be changed.
 */
void rule_copy(Rule * const destination, const Rule * const source) {
    if ((destination != NULL) && (source != NULL)) {
        if ((source->body != NULL) && (source->head.atom != NULL)) {
            destination->body_size = source->body_size;
            literal_copy(&(destination->head), &(source->head));

            destination->body = (Literal *) malloc(source->body_size * sizeof(Literal));
            unsigned int i;
            for (i = 0; i < source->body_size; ++i) {
                literal_copy(&(destination->body[i]), &(source->body[i]));
            }
            destination->weight = source->weight;
        }
    }
}

/**
 * @brief Promote the rule by adding the given amount. If the amount given is negative, it will be 
 * considered a demotion.
 * 
 * @param rule The Rule to be promoted. If NULL, nothing will happen.
 * @param amount The amount to be added to the weight of the rule.
 */
void rule_promote(Rule * const rule, const float amount) {
    if (rule != NULL) {
        rule->weight = fmaxf(rule->weight + amount, 0);
    }
}

/**
 * @brief Demote the rule by subtracting the given amount. If the amount is negative, it will be 
 * considered a promotion. The weight will never be negative.
 * 
 * @param rule The Rule to be demoted. If NULL, nothing will happen.
 * @param amount The amount to be subtracted from the weight of the rule.
 */
void rule_demote(Rule * const rule, const float amount) {
    if (rule != NULL) {
        rule->weight = fmaxf(rule->weight - amount, 0);
    }
}

//TODO Create a test function.
/**
 * @brief Checks if a Rule is applicable with a given context. An applicable Rule, is a Rule whose 
 * body is true in the given Context.
 * 
 * @param rule The Rule to be checked.
 * @param context The Context that the Rule will be assest with.
 * @return 1 if the Rule is applicable, 0 if it is not, or -1 if at least one of the given 
 * parameters is NULL.
 */
int rule_applicable(const Rule * restrict rule, const Context * restrict context) {
    if ((rule != NULL) && (context != NULL)) {
        unsigned int i, j, applicable_literals = 0;
        for (i = 0; i < context->size; ++i) {
            for (j = 0; j < rule->body_size; ++j) {
                if (literal_equals(&(context->observations[i]), &(rule->body[j]))) {
                    ++applicable_literals;
                    if (applicable_literals == rule->body_size) {
                        return 1;
                    }
                }
            }
        }
        return 0;
    }
    return -1;
}

//TODO Create a test function.
/**
 * @brief Check if a Rule concurs with a given context. A concurring Rule, is a Rule whose head is 
 * true in the given context, and it's body doesn't have to be.
 * 
 * @param rule The Rule to be checked.
 * @param context The Context that the Rule will be assest with.
 * @return 1 if the Rule concurs; i.e. the head is true, 0 if the head does not concur, and -1 if 
 * one of given parameters is NULL.
*/
int rule_concurs(const Rule * restrict rule, const Context * restrict context) {
    if (rule && context) {
        unsigned int i;
        for (i = 0; i < context->size; ++i) {
            if (literal_equals(&(rule->head), &(context->observations[i]))) {
                return 1;
            }
        }
        return 0;
    }
    return -1;
}

/**
 * @brief Check two Rules to see if they are equal. It check their body, and their head. Their body 
 * Literals can be in different order.
 * 
 * @param rule1 The first rule to be checked.
 * @param rule2 The second rule to be checked.
 * @return 1 if they are equal, 0 if they are not and -1 of one of the Rules is NULL.
 */
int rule_equals(const Rule * const rule1, const Rule * const rule2) {
    if ((rule1 != NULL) && (rule2 != NULL)) {
        if (rule1->body_size == rule2->body_size) {
            if (literal_equals(&(rule1->head), &(rule2->head))) {
                unsigned int i, j;
                unsigned short failed = 0;
                for (i = 0; i < rule1->body_size; ++i) {
                    for (j = 0; j < rule2->body_size; ++j) {
                        if (!literal_equals(&(rule1->body[i]), &(rule2->body[j]))) {
                            ++failed;
                        }
                    }

                    if (failed == rule1->body_size) {
                        return 0;
                    } else {
                        failed = 0;
                    }
                }
                return 1;
            }
        }
        return 0;
    }
    return -1;
}

/**
 * @brief Converts the Rule into a string format.
 * 
 * @param rule The Rule to be converted.
 * @return The string format of the given Rule. Use free() to deallocate this string. Returns NULL 
 * if the Rule, its body or the head's atom are NULL.
 */
char *rule_to_string(const Rule * const rule) {
    if (rule != NULL) {
        if ((rule->body != NULL) && (rule->head.atom != NULL)) {
            unsigned int i;
            char *literal_string, *result = strdup("(");
            size_t result_size = strlen(result) + 1;

            literal_string = literal_to_string(&(rule->body[0]));
            result_size += strlen(literal_string);
            char *temp = strdup(result);
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s%s", temp, literal_string);
            free(temp);
            free(literal_string);

            for (i = 1; i < rule->body_size; ++i) {
                literal_string = literal_to_string(&(rule->body[i]));
                result_size += strlen(literal_string) + 2;
                temp = strdup(result);
                result = (char *) realloc(result, result_size);
                sprintf(result, "%s, %s", temp, literal_string);
                free(literal_string);
                free(temp);
            }

            char weight_length[50];
            int weight_size = sprintf(weight_length, "%.4f", rule->weight);

            literal_string = literal_to_string(&rule->head);
            result_size += strlen(literal_string) + weight_size + 8;
            temp = strdup(result);
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s) => %s (%.4f)", temp, literal_string, rule->weight);
            free(literal_string);
            free(temp);
            return result;
        }
    }
    return NULL;
}
/**
 * @brief Converts a Rule to a Prudens JS Rule format.
 * 
 * @param rule The Rule to be converted.
 * @param rule_number A number to be appended at the name of the rule.
 * @return The Prudens JS Rule format (as a string) of the given Rule. Use free() to deallocate the 
 * result. Returns NULL if the Rule, its body or the head's atom are NULL.
 */
char *rule_to_prudensjs(const Rule * const rule, const unsigned int rule_number) {
    if (rule != NULL) {
        if ((rule->body != NULL) && (rule->head.atom != NULL)) {
            char temp_buffer[50];
            int rule_number_size = sprintf(temp_buffer, "%d", rule_number);

            const char * const start = "{\"name\": \"Rule";
            char *result, *body = strdup("\", \"body\": ["), *temp, *literal_prudensjs_string;
            size_t body_size = strlen(body) + 1, result_size;

            unsigned int i;
            for (i = 0; i < rule->body_size - 1; ++i) {
                literal_prudensjs_string = literal_to_prudensjs(&(rule->body[i]));
                body_size += strlen(literal_prudensjs_string) + 2;
                temp = strdup(body);
                body = (char *) realloc(body, body_size);
                sprintf(body, "%s%s, ", temp, literal_prudensjs_string);
                free(temp);
                free(literal_prudensjs_string);
            }

            literal_prudensjs_string = literal_to_prudensjs(&(rule->body[i]));
            body_size += strlen(literal_prudensjs_string) + 11;
            temp = strdup(body);
            body = (char *) realloc(body, body_size);
            sprintf(body, "%s%s], \"head\": ", temp, literal_prudensjs_string);
            free(temp);
            free(literal_prudensjs_string);

            literal_prudensjs_string = literal_to_prudensjs(&(rule->head));

            result_size = strlen(start) + rule_number_size + body_size +
            strlen(literal_prudensjs_string) + 1;

            result = (char *) malloc(result_size);

            sprintf(result, "%s%d%s%s}", start, rule_number, body, literal_prudensjs_string);
            free(literal_prudensjs_string);
            free(body);

            return result;
        }
    }
    return NULL;
}