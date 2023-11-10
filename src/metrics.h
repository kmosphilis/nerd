#ifndef METRICS_H
#define METRICS_H
#include <stdlib.h>

#include "nerd.h"
#include "scene.h"
#include "context.h"
#include "nerd_helper.h"

int evaluate_all_literals(const Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const file_to_evaluate, size_t * const restrict total_hidden,
size_t * const restrict total_recovered, size_t * const restrict total_incorrectly_recovered,
size_t * const restrict total_not_recovered);
int evaluate_random_literals(const Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const file_to_evaluate, const float ratio, size_t * const restrict total_hidden,
size_t * const restrict total_recovered, size_t * const restrict total_incorrectly_recovered,
size_t * const restrict total_not_recovered);
int evaluate_labels(const Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const file_to_evaluate, const Context * const labels, const char delimiter,
const bool has_header, float * const restrict accuracy, float * const restrict abstain_ratio,
size_t * const total_observations, Scene *** const restrict observations,
Scene *** const restrict inferences, char ** const save_inferring_rules);

#endif
