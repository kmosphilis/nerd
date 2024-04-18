#ifndef METRICS_H
#define METRICS_H
#include <stdlib.h>

#include "nerd.h"
#include "scene.h"
#include "context.h"
#include "nerd_helper.h"

int evaluate_all_literals(const Nerd * const nerd, void (*inference_engine)
(const KnowledgeBase * const knowledge_base, const Scene * const restrict observation,
Scene **inference), const Sensor * const file_to_evaluate, size_t * const restrict total_hidden,
size_t * const restrict total_recovered, size_t * const restrict total_incorrectly_recovered,
size_t * const restrict total_not_recovered);
int evaluate_random_literals(const Nerd * const nerd, void (*inference_engine)
(const KnowledgeBase * const knowledge_base, const Scene * const restrict observation,
Scene **inference), const Sensor * const file_to_evaluate, const float ratio,
size_t * const restrict total_hidden, size_t * const restrict total_recovered,
size_t * const restrict total_incorrectly_recovered, size_t * const restrict total_not_recovered);
int evaluate_labels(const Nerd * const nerd, int (*inference_engine_batch)
(const KnowledgeBase * const knowledge_base, const size_t total_observations,
Scene ** restrict observation, Scene *** const inference, char ** const save_inferring_rules),
const Sensor * const file_to_evaluate, const Context * const labels,
float * const restrict accuracy, float * const restrict abstain_ratio,
size_t * const total_observations, Scene *** const restrict observations,
Scene *** const restrict inferences, char ** const save_inferring_rules,
bool partial_observation);

#endif
