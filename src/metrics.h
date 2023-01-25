#include "nerd.h"

#ifndef __METRICS_H__
#define __METRICS_H__

void evaluate_all_literals(const Nerd * const nerd, const Scene * const observation,
float * const overall_success);
// void evaluate_hidden_literals(const Nerd * const nerd, const Scene * const observation,
// const float ratio_of_hidden_literals, float * const overall_completeness);

#endif
