#include <float.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#include "nerd_utils.h"
#include "rule.h"

/**
 * @brief Constructs a Rule.
 *
 * @param body_size The size of the body of the Rule.
 * @param body An array containing a series of Literals required to activate the
 * Rule. It should be an array of Literal * (or a Literal **). Upon succession,
 * the items in this array/pointer paremter will become NULL, if and only if
 * take_ownership is true.
 * @param head The head of the Rule when it gets activated. It should be a
 * reference to a Literal * (Literal ** - a pointer to a Literal *). Upon
 * succession, this parameter will become NULL, if and only if take_ownership is
 * true.
 * @param weight The weight of the rule.
 * @param take_ownership Indicates whether the Rule should take onwership of the
 * given Literals (body and head) that will be added or just keep their
 * reference. If true is given it will take their ownership, otherwise it will
 * not.
 *
 * @return A new Rule * or NULL, if the body is NULL, the body_size is > 0 or
 * the head is NULL. Use rule_destructor to deallocate.
 */
Rule *rule_constructor(const unsigned int body_size, Literal **const body,
                       Literal **const head, const float weight,
                       const bool take_ownership) {
  if (head && (*head) && body && (body_size > 0)) {
    Rule *rule = (Rule *)malloc(sizeof(Rule));

    rule->head = *head;
    if (take_ownership) {
      *head = NULL;
    }
    rule->body = context_constructor(take_ownership);

    unsigned int i;
    for (i = 0; i < body_size; ++i) {
      context_add_literal(rule->body, &(body[i]));
    }

    rule->weight = weight;
    return rule;
  }
  return NULL;
}

/**
 * @brief Deconstructs a Rule.
 *
 * @param rule The Rule to be destructed. It should be a reference to the
 * struct's pointer (to a Rule *).
 */
void rule_destructor(Rule **const rule) {
  if (rule && (*rule)) {
    if (scene_is_taking_ownership((*rule)->body)) {
      literal_destructor(&((*rule)->head));
    }
    context_destructor(&((*rule)->body));
    (*rule)->weight = INFINITY;
    safe_free(*rule);
  }
}

/**
 * @brief Makes a copy of the given Rule.
 *
 * @param destination The Rule to save the copy. It should be a reference to the
 * struct's pointer (to a Rule *).
 * @param source The Rule to be copied. If the Rule, its body or the head's atom
 * are NULL, the content of the destination will not be changed.
 */
void rule_copy(Rule **const destination, const Rule *const restrict source) {
  if (destination && source) {
    *destination = (Rule *)malloc(sizeof(Rule));
    if (scene_is_taking_ownership(source->body)) {
      literal_copy(&((*destination)->head), source->head);
    } else {
      (*destination)->head = source->head;
    }
    scene_copy(&((*destination)->body), source->body);
    (*destination)->weight = source->weight;
  }
}

/**
 * @brief Shows whether the given Rule took the ownership of the Literals from
 * its body and head.
 *
 * @param rule The Rule to find if it took ownership or not.
 *
 * @return 1 (true) if it took ownership, 0 (false) if it didn't not or -1 if
 * the rule is NULL.
 */
int rule_took_ownership(const Rule *const rule) {
  if (rule) {
    return scene_is_taking_ownership(rule->body);
  }
  return -1;
}

/**
 * @brief Promote the rule by adding the given amount. If the amount given is
 * negative, it will be considered a demotion.
 *
 * @param rule The Rule to be promoted. If NULL, nothing will happen.
 * @param amount The amount to be added to the weight of the rule.
 */
void rule_promote(Rule *const rule, const float amount) {
  if (rule) {
    rule->weight = fmaxf(rule->weight + amount, 0);
  }
}

/**
 * @brief Demote the rule by subtracting the given amount. If the amount is
 * negative, it will be considered a promotion. The weight will never be
 * negative.
 *
 * @param rule The Rule to be demoted. If NULL, nothing will happen.
 * @param amount The amount to be subtracted from the weight of the rule.
 */
void rule_demote(Rule *const rule, const float amount) {
  if (rule) {
    rule->weight = fmaxf(rule->weight - amount, 0);
  }
}

/**
 * @brief Checks if a Rule is applicable with a given context. An applicable
 * Rule, is a Rule whose body is true in the given Context.
 *
 * @param rule The Rule to be checked.
 * @param context The Context that the Rule will be assest with.
 *
 * @return 1 if the Rule is applicable, 0 if it is not, or -1 if at least one of
 * the given parameters is NULL.
 */
int rule_applicable(const Rule *const rule, const Context *const context) {
  if (rule && context) {
    unsigned int i, j, applicable_literals = 0;
    for (i = 0; i < context->size; ++i) {
      for (j = 0; j < rule->body->size; ++j) {
        if (literal_equals(context->literals[i], rule->body->literals[j])) {
          ++applicable_literals;
          if (applicable_literals == rule->body->size) {
            return 1;
          }
        }
      }
    }
    return 0;
  }
  return -1;
}

/**
 * @brief Check if a Rule concurs with a given context. A concurring Rule, is a
 * Rule whose head is true in the given context, and it's body doesn't have to
 * be.
 *
 * @param rule The Rule to be checked.
 * @param context The Context that the Rule will be assest with.
 *
 * @return 1 if the Rule concurs; i.e. the head is true, 0 if the head does not
 * concur, and -1 if one of given parameters is NULL.
 */
int rule_concurs(const Rule *const rule, const Context *const context) {
  if (rule && context) {
    unsigned int i;
    for (i = 0; i < context->size; ++i) {
      if (literal_equals(rule->head, context->literals[i])) {
        return 1;
      }
    }
    return 0;
  }
  return -1;
}

/**
 * @brief Check two Rules to see if they are equal. It check their body, and
 * their head. Their body Literals can be in different order.
 *
 * @param rule1 The first rule to be checked.
 * @param rule2 The second rule to be checked.
 *
 * @return 1 if they are equal, 0 if they are not and -1 of one of the Rules is
 * NULL.
 */
int rule_equals(const Rule *const restrict rule1,
                const Rule *const restrict rule2) {
  if (rule1 && rule2) {
    if (rule1->body->size == rule2->body->size) {
      if (literal_equals(rule1->head, rule2->head)) {
        unsigned int i, j;
        unsigned short failed = 0;
        for (i = 0; i < rule1->body->size; ++i) {
          for (j = 0; j < rule2->body->size; ++j) {
            if (!literal_equals(rule1->body->literals[i],
                                rule2->body->literals[j])) {
              ++failed;
            }
          }

          if (failed == rule1->body->size) {
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
 *
 * @return The string format of the given Rule. Use free() to deallocate this
 * string. Returns NULL if the Rule, its body or the head's atom are NULL.
 */
char *rule_to_string(const Rule *const rule) {
  if (rule) {
    if (rule->body && (rule->body->size != 0) && rule->head) {
      char *literal_string, *result = strdup("(");
      size_t result_size = strlen(result) + 1;

      literal_string = literal_to_string(rule->body->literals[0]);
      result_size += strlen(literal_string);
      char *temp = strdup(result);
      result = (char *)realloc(result, result_size);
      sprintf(result, "%s%s", temp, literal_string);
      free(temp);
      free(literal_string);

      unsigned int i;
      for (i = 1; i < rule->body->size; ++i) {
        literal_string = literal_to_string(rule->body->literals[i]);
        result_size += strlen(literal_string) + 2;
        temp = strdup(result);
        result = (char *)realloc(result, result_size);
        sprintf(result, "%s, %s", temp, literal_string);
        free(literal_string);
        free(temp);
      }

      char weight_length[50];
      int weight_size = sprintf(weight_length, "%.4f", rule->weight);

      literal_string = literal_to_string(rule->head);
      result_size += strlen(literal_string) + weight_size + 8;
      temp = strdup(result);
      result = (char *)realloc(result, result_size);
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
 *
 * @return The Prudens JS Rule format (as a string) of the given Rule. Use
 * free() to deallocate the result. Returns NULL if the Rule, its body or the
 * head's atom are NULL.
 */
char *rule_to_prudensjs(const Rule *const rule,
                        const unsigned int rule_number) {
  if (rule) {
    if (rule->body && (rule->body->size != 0) && rule->head) {
      char temp_buffer[50];
      int rule_number_size = sprintf(temp_buffer, "%d", rule_number);

      const char *const start = "{\"name\": \"Rule";
      char *result, *body = strdup("\", \"body\": ["), *temp,
                    *literal_prudensjs_string;
      size_t body_size = strlen(body) + 1, result_size;

      unsigned int i;
      for (i = 0; i < rule->body->size - 1; ++i) {
        literal_prudensjs_string =
            literal_to_prudensjs(rule->body->literals[i]);
        body_size += strlen(literal_prudensjs_string) + 2;
        temp = strdup(body);
        body = (char *)realloc(body, body_size);
        sprintf(body, "%s%s, ", temp, literal_prudensjs_string);
        free(temp);
        free(literal_prudensjs_string);
      }

      literal_prudensjs_string = literal_to_prudensjs(rule->body->literals[i]);
      body_size += strlen(literal_prudensjs_string) + 11;
      temp = strdup(body);
      body = (char *)realloc(body, body_size);
      sprintf(body, "%s%s], \"head\": ", temp, literal_prudensjs_string);
      free(temp);
      free(literal_prudensjs_string);

      literal_prudensjs_string = literal_to_prudensjs(rule->head);

      result_size = strlen(start) + rule_number_size + body_size +
                    strlen(literal_prudensjs_string) + 1;

      result = (char *)malloc(result_size);

      sprintf(result, "%s%d%s%s}", start, rule_number, body,
              literal_prudensjs_string);
      free(literal_prudensjs_string);
      free(body);

      return result;
    }
  }
  return NULL;
}
