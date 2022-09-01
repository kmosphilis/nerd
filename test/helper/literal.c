#include <check.h>

#include "literal.h"

/**
 * @brief Check two Literals to determine if they are equal (literal1 == literal2).
 * 
 * @param literal1 The first Literal to compare.
 * @param literal2 The second Literal to compare.
 */
void ck_assert_literal_eq(const Literal * const literal1, const Literal * const literal2) {
    ck_assert_int_eq(literal1->sign, literal2->sign);
    ck_assert_pstr_eq(literal1->atom, literal2->atom);
}

/**
 * @brief Check two Literals to determine if they are not equal (literal1 != literal2).
 * 
 * @param literal1 The first Literal to compare.
 * @param literal2 The second Literal to compare.
 */
void ck_assert_literal_ne(const Literal * const literal1, const Literal * const literal2) {
    if ((literal1->atom != NULL) && (literal2->atom != NULL)) {
        if (strcmp(literal1->atom, literal2->atom) == 0) {
            ck_assert_int_ne(literal1->sign, literal2->sign);
        } else {
            ck_assert_str_ne(literal1->atom, literal2->atom);
        }
    }
}

/**
 * @brief Check if a Literal is not empty.
 * 
 * @param literal The Literal to check.
 */
void ck_assert_literal_notempty(const Literal * const literal1) {
    ck_assert_int_ne(literal1->sign, -1);
    ck_assert_ptr_nonnull(literal1->atom);
}

/**
 * @brief Check if a Literal is empty.
 * 
 * @param literal The Literal to check.
 */
void ck_assert_literal_empty(const Literal * const literal1) {
    ck_assert_int_eq(literal1->sign, -1);
    ck_assert_ptr_null(literal1->atom);
}