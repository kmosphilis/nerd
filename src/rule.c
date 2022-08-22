#include <malloc.h>
#include <math.h>
#include <float.h>
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
void rule_construct(Rule * const rule, const unsigned int body_size, Literal ** const body,
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
            literal_destruct(&(rule->head));
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
void rule_destruct(Rule * const rule) {
    if (rule != NULL) {
        literal_destruct(&(rule->head));
        if (rule->body != NULL) {
            unsigned int i;
            for (i = 0; i < rule->body_size; ++i) {
                literal_destruct(&(rule->body[i]));
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
 * @brief Converts the Rule into a string format.
 * 
 * @param rule The Rule to be converted.
 * @return The string format of the given Rule. Use free() to deallocate this string. Returns NULL 
 * if the Rule, its body or the head's atom are empty (NULL).
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
 * @brief Promote the rule by adding the given amount. If the amount given is negative, it will be 
 * considered a demotion.
 * 
 * @param rule The Rule to be promoted. If NULL, nothing will happen.
 * @param amount The amount to be added to the weight of the rule.
 */
void rule_promote(Rule * const rule, const float amount) {
    if (rule != NULL) {
        rule->weight += amount;
    }
}

/**
 * @brief Demote the rule by subtracting the given amount. If the amount is negative, it will be 
 * considered a promotion.
 * 
 * @param rule The RUle to be demoted. If NULL, nothing will happen.
 * @param amount The amount to be subtracted from the weight of the rule.
 */
void rule_demote(Rule * const rule, const float amount) {
    if (rule != NULL) {
        rule->weight -= amount;
    }
}