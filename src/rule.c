#include <malloc.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "rule.h"

/**
 * @brief Constructs a Rule.
 * 
 * @param rule The Rule to be constructed.
 * @param body_size The size of the body of the rule.
 * @param body An array containing a series of Literals required to activate the rule.
 * @param head The head when the rule gets activated.
 * @param weight The weight of the rule.
 */
void rule_construct(Rule * const rule, const int body_size, Literal ** const body,
    const Literal * const head, const float weight) {
    rule->body_size = body_size;
    if (head != NULL) {
        literal_copy(&(rule->head), head);
    }
    rule->body = (Literal *) malloc(sizeof(Literal) * body_size);
    int i;
    if (body != NULL) {
        for (i = 0; i < body_size; ++i) {
            literal_copy(&(rule->body[i]), &(*body)[i]);
        }
    }
    rule->weight = weight;
}

/**
 * @brief Deconstructs a Rule.
 * 
 * @param rule The Rule to be destructed.
 */
void rule_destruct(Rule * const rule) {
    literal_destruct(&(rule->head));
    if (rule->body != NULL) {
        int i;
        for (i = 0; i < rule->body_size; ++i) {
            literal_destruct(&(rule->body[i]));
        }
        free(rule->body);
        rule->body = NULL;
    }
    rule->body_size = 0;
    rule->weight = INFINITY;
}

/**
 * @brief Makes a copy of the given Rule.
 * 
 * @param destination The Rule to save the copy.
 * @param source The Rule to be copied.
 */
void rule_copy(Rule * const destination, const Rule * const source) {
    destination->body_size = source->body_size;
    literal_copy(&destination->head, &source->head);

    destination->body = (Literal *) malloc(sizeof(Literal) * source->body_size);
    int i;
    for (i = 0; i < source->body_size; ++i) {
        literal_copy(&(destination->body[i]), &(source->body[i]));
    }
    destination->weight = source->weight;
}

/**
 * @brief Converts the Rule into a string format.
 * 
 * @param rule The Rule to be converted.
 * @return The string format of the given Rule. Use free() to deallocate this string.
 */
char *rule_to_string(const Rule * const rule) {
    int i;
    char *literal_string, *result = strdup("(");
    size_t result_size = strlen(result);

    Literal current_literal = rule->body[0];
    size_t literal_length = strlen(current_literal.atom);
    result_size += literal_length;
    result = (char *) realloc(result, result_size);
    sprintf(result, "%s%s", result, current_literal.atom);

    for (i = 1; i < rule->body_size; ++i) {
        Literal current_literal = rule->body[i];
        literal_string = literal_to_string(&current_literal);
        size_t literal_length = strlen(literal_string);
        result_size += literal_length + 2;
        result = (char *) realloc(result, result_size);
        sprintf(result, "%s, %s", result, literal_string);
        free(literal_string);
    }

    char weight_length[50];
    int weight_size = sprintf(weight_length, "%.4f", rule->weight);

    literal_string = literal_to_string(&rule->head);
    result_size += (strlen(literal_string) + weight_size + 8);
    result = (char *) realloc(result, result_size);
    sprintf(result, "%s) => %s (%.4f)", result, literal_string, rule->weight);
    free(literal_string);
    return result;
}

/**
 * @brief Promote the rule by adding the given amount. If the amount given is negative, it will be 
 * considered a demotion.
 * 
 * @param rule The Rule to be promoted.
 * @param amount The amount to be added to the weight of the rule.
 */
void rule_promote(Rule * const rule, const float amount) {
    rule->weight += amount;
}

/**
 * @brief Demote the rule by subtracting the given amount. If the amount is negative, it will be 
 * considered a promotion.
 * 
 * @param rule The RUle to be demoted.
 * @param amount The amount to be subtracted from the weight of the rule.
 */
void rule_demote(Rule * const rule, const float amount) {
    rule->weight -= amount;
}