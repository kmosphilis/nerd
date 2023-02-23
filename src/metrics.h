#include "nerd.h"

#ifndef METRICS_H
#define METRICS_H

void evaluate_all_literals(const Nerd * const nerd, const Scene * const observation,
float * const overall_success);
// void evaluate_hidden_literals(const Nerd * const nerd, const Scene * const observation,
// const float ratio_of_hidden_literals, float * const overall_completeness);

#endif
