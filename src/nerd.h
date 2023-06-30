#ifndef NERD_H
#define NERD_H

#include <stdbool.h>

#include "sensor.h"
#include "knowledge_base.h"
#include "nerd_helper.h"

typedef struct Nerd {
    Sensor *sensor;
    KnowledgeBase *knowledge_base;
    size_t breadth, depth, epochs;
    float promotion_weight, demotion_weight;
    bool partial_observation;
} Nerd;

Nerd *nerd_constructor(const char * const filepath, const char delimiter, const bool reuse,
const bool header , const float activation_threshold, const unsigned int breadth,
const unsigned int depth, const unsigned int epochs, const float promotion_weight,
const float demotion_weight, const bool use_backward_chaining, const bool partial_observation);
Nerd *nerd_constructor_from_file(const char * const filepath, const unsigned int epochs,
const bool use_backward_chaining, const bool reuse_sensor);
void nerd_destructor(Nerd ** const nerd);
void nerd_start_learning(Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const test_directory, const char * const evaluation_filepath,
const Context * const restrict labels);
void nerd_to_string(const Nerd * const nerd);
void nerd_to_file(const Nerd * const nerd, const char * const filepath);

#endif
