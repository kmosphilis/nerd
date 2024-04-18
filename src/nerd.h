#ifndef NERD_H
#define NERD_H

#include <stdbool.h>

#include "sensor.h"
#include "knowledge_base.h"
#include "nerd_helper.h"

typedef struct Nerd {
    KnowledgeBase *knowledge_base;
    size_t max_rules_per_instance, breadth, depth;
    float promotion_weight, demotion_weight;
    bool increasing_demotion;
} Nerd;

Nerd *nerd_constructor(const float activation_threshold, const unsigned int max_rules_per_instance,
const unsigned int breadth, const unsigned int depth, const float promotion_weight,
const float demotion_weight, const bool use_backward_chaining, const bool increasing_demotion);
Nerd *nerd_constructor_from_file(const char * const filepath, const bool use_backward_chaining);
void nerd_destructor(Nerd ** const nerd);
void nerd_train(Nerd * const nerd, void (*inference_engine)
(const KnowledgeBase * const knowledge_base, const Scene * const restrict observation,
Scene **inference), const Scene * const restrict observation,
const Context * const restrict labels, size_t * const nerd_time_taken, size_t * const ie_time_taken,
char **header, const size_t header_size, Scene **incompatibilities);
void nerd_to_string(const Nerd * const nerd);
void nerd_to_file(const Nerd * const nerd, const char * const filepath);

#endif
