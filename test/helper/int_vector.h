#include "../../src/int_vector.h"

#ifndef __INT_VECTOR_HELPER_H__
#define __INT_VECTOR_HELPER_H__

void ck_assert_int_vector_eq(const IntVector * restrict int_vector_1,
const IntVector * restrict int_vector_2);
void ck_assert_int_vector_empty(const IntVector * restrict int_vector);
void ck_assert_int_vector_notempty(const IntVector * restrict int_vector);
#endif