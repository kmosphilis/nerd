#include <stdbool.h>

#include "sensor.h"
#include "knowledge_base.h"

#ifndef NERD_H
#define NERD_H

typedef struct Nerd {
    Sensor *sensor;
    KnowledgeBase *knowledge_base;
    size_t breadth, depth, epochs;
    float promotion_weight, demotion_weight;
    bool partial_observation;
} Nerd;

Nerd *nerd_constructor(const char * const filepath, const bool reuse,
const float activation_threshold, const unsigned int breadth, const unsigned int depth,
const unsigned int epochs, const float promotion_weight, const float demotion_weight,
const bool partial_observation);
Nerd *nerd_constructor_from_file(const char * const filepath, const unsigned int epochs);
void nerd_destructor(Nerd ** const nerd);
void prudensjs_inference(const KnowledgeBase * const knowledge_base,
const Scene * const restrict observation, Scene ** const inferred);
void nerd_start_learning(Nerd * const nerd);
void nerd_to_string(const Nerd * const nerd);
void nerd_to_file(const Nerd * const nerd, const char * const filepath);

#endif
