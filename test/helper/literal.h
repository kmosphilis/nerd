#include "../../src/literal.h"

#ifndef __LITERAL_HELPER_H__
#define __LITERAL_HELPER_H__

void ck_assert_literal_eq(const Literal * const literal1, const Literal * const literal2);
void ck_assert_literal_ne(const Literal * const literal1, const Literal * const literal2);
void ck_assert_literal_notempty(const Literal * const literal1);
void ck_assert_literal_empty(const Literal * const literal1);

#endif