#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "literal.h"

/**
 * @brief Constructs a Literal. If the atom is NULL, the Literal that will be created will be in a 
 * destructed, unusable form.
 * 
 * @param literal The Literal to be constructed.
 * @param atom The name of the atom to be used.
 * @param sign Indicates whether the atom is negated or not. > 0 is positive, 0 is negative.
 */
void literal_construct(Literal * const literal, const char * const atom, const int sign) {
    if (literal != NULL) {
        if (atom != NULL) {
            literal->atom = strdup(atom);
            literal->sign = sign > 0;
        } else {
            literal->atom = NULL;
            literal->sign = -1;
        }
    }
}

/**
 * @brief Destructs the given Literal.
 * 
 * @param literal The Literal to be destructed.
 */
void literal_destruct(Literal * const literal) {
    if (literal != NULL) {
        if (literal->atom != NULL) {
            free(literal->atom);
            literal->atom = NULL;
        }
        literal->sign = -1;
    }
}

/**
 * @brief Makes a copy of the given Literal.
 * 
 * @param destination The Literal to save the copy.
 * @param source The Literal to be copied. If the Literal or its atom are NULL, the content of the 
 * destination will not be changed.
 */
void literal_copy(Literal * const destination, const Literal * const source) {
    if ((destination != NULL) && (source != NULL)) {
        if (source->atom != NULL) {
            destination->atom = strdup(source->atom);
            destination->sign = source->sign;
        }
    }
}

/**
 * @brief Converts the Literal to a string format with it's sign (positive or negative).
 * 
 * @param literal The Literal to be converted.
 * @return The string format of the given Literal. Use free() to deallocate this string. Returns 
 * NULL if the Literal or its atom are empty (NULL).
 */
char *literal_to_string(const Literal * const literal) {
    if (literal != NULL) {
        if (literal->atom != NULL) {
            if (literal->sign > 0) {
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
 * @brief Negates the given Literal. If it is positive it will become negative, and vice versa.
 * 
 * @param literal The Literal to negate. If NULL, nothing will happen.
 */
void literal_negate(Literal * const literal) {
    if (literal != NULL) {
        if (literal->sign) {
            literal->sign = 0;
        } else {
            literal->sign = 1;
        }
    }
}

/**
 * @brief Check two Literals to see if they are equal (literal1 == literal2).
 * 
 * @param literal1 The first literal to be checked.
 * @param literal2 The second literal to be checked.
 * @return 1 if they are equal, 0 if they are not and -1 if one the Literals is NULL.
 */
int literal_equals(const Literal * const literal1, const Literal * const literal2) {
    if ((literal1 != NULL) && (literal2 != NULL)) {
        if (literal1->sign == literal2->sign) {
            return (strcmp(literal1->atom, literal2->atom) == 0);
        }
        return 0;
    }
    return -1;
}