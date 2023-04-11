#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "literal.h"

/**
 * @brief Constructs a Literal. The atom's characters will be converted to their lowercase form.
 *
 * @param atom The name of the atom to be used.
 * @param sign Indicates whether the atom is negated or not. > 0 (true) is positive, 0 (false) is
 * negative.
 *
 * @return A new Literal * or NULL if atom == NULL. Use literal_destructor to deallocate.
 */
Literal *literal_constructor(const char * const atom, const bool sign) {
    if (atom) {
        Literal *literal = (Literal *) malloc(sizeof(Literal));
        literal->atom = strdup(atom);
        unsigned int i;
        for (i = 0; i < strlen(atom); ++i) {
            literal->atom[i] = tolower(literal->atom[i]);
        }
        literal->sign = sign > 0;
        return literal;
    }
    return NULL;
}

/**
 * @brief Constructs a Literal from a string (char *). The atom's characters will be converted to
 * their lowercase form.
 *
 * @param string The string that the Literal will be constructed from. For a negated Literal include
 * a dash (-) in the beginning.
 *
 * @return A new Literal * or NULL if string == NULL. Use literal_destructor to deallocate.
 */
Literal *literal_constructor_from_string(const char * const string) {
    if (string) {
        Literal *literal;
        if (string[0] == '-') {
            literal = literal_constructor(string + 1, false);
        } else {
            literal = literal_constructor(string, true);
        }
        return literal;
    }
    return NULL;
}

/**
 * @brief Destructs the given Literal.
 *
 * @param literal The Literal to be destructed. It should be a reference to the struct's pointer (to
 * a Literal *).
 */
void literal_destructor(Literal ** const literal) {
    if (literal && (*literal)) {
        if ((*literal)->atom) {
            free((*literal)->atom);
            (*literal)->atom = NULL;
        }
        (*literal)->sign = 0;
        free(*literal);
        *literal = NULL;
    }
}

/**
 * @brief Makes a copy of the given Literal.
 *
 * @param destination The Literal to save the copy. It should be a reference to the struct's
 * pointer (to a Literal *).
 * @param source The Literal to be copied. If the Literal or its atom are NULL, the content of the
 * destination will not be changed.
 */
void literal_copy(Literal ** const destination, const Literal * const restrict source) {
    if (destination && source) {
        *destination = literal_constructor(source->atom, source->sign);
    }
}

/**
 * @brief Negates the given Literal. If it is positive it will become negative, and vice versa.
 *
 * @param literal The Literal to negate. If NULL, nothing will happen.
 */
void literal_negate(Literal * const literal) {
    if (literal) {
        if (literal->sign) {
            literal->sign = false;
        } else {
            literal->sign = true;
        }
    }
}

/**
 * @brief Check two Literals to see if they are equal (literal1 == literal2).
 *
 * @param literal1 The first literal to be checked.
 * @param literal2 The second literal to be checked.
 *
 * @return 1 if they are equal, 0 if they are not and -1 if one the Literals is NULL.
 */
int literal_equals(const Literal * const restrict literal1,
const Literal * const restrict literal2) {
    if (literal1 && literal2) {
        if (literal1->sign == literal2->sign) {
            return (strcmp(literal1->atom, literal2->atom) == 0);
        }
        return 0;
    }
    return -1;
}

/**
 * @brief Check two Literals to see if their atoms are equal, and their signs are opposed. If they
 * are not opposed, it does not imply that they are equal.
 *
 * @param literal1 The first Literal to be checked.
 * @param literal2 The second Literal to be checked.
 *
 * @return 1 if they are opposed, 0 if they are not opposed, -1 if the atoms are different, and -2
 * if at least one of the Literals is NULL.
 */
int literal_opposed(const Literal * const restrict literal1,
const Literal * const restrict literal2) {
    if (literal1 && literal2) {
        if (strcmp(literal1->atom, literal2->atom) == 0) {
            return (literal1->sign != literal2->sign);
        }
        return -1;
    }
    return -2;
}

/**
 * @brief Converts the Literal to a string format with it's sign (positive or negative).
 *
 * @param literal The Literal to be converted.
 *
 * @return The string format of the given Literal. Use free() to deallocate this string. Returns
 * NULL if the Literal or its atom are NULL.
 */
char *literal_to_string(const Literal * const literal) {
    if (literal) {
        if (literal->atom) {
            if (literal->sign) {
                return strdup(literal->atom);
            }

            char *result = (char *) malloc(strlen(literal->atom) + 2);
            sprintf(result, "-%s", literal->atom);
            return result;
        }
    }
    return NULL;
}

/**
 * @brief Converts a Literal to a Prudens JS Literal format.
 *
 * @param literal The Literal to be converted.
 *
 * @return The Prudens JS Literal format (as a string) of the given Literal. Use free() to
 * deallocate the result. Returns NULL if the Literal or its atom are NULL.
 */
char *literal_to_prudensjs(const Literal * const literal) {
    if (literal) {
        if (literal->atom) {
            const char * const start = "{\"name\": \"", * const sign = ", \"sign\": ",
            * const end = ", \"isJS\": false, \"isEquality\": false, \"isInEquality\": false, "
            "\"isAction\": false, \"arity\": 0}";
            char *result;
            size_t result_size = strlen(start) + strlen(literal->atom) + strlen(sign) +
            (literal->sign ? 4 : 5) + strlen(end) + 2;

            result = (char *) malloc(result_size);
            sprintf(result, "%s%s\"%s%s%s", start, literal->atom, sign,
            literal->sign ? "true" : "false", end);

            return result;
        }
    }
    return NULL;
}
