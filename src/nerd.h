#include <stdint.h>

#include "sensor.h"
#include "knowledge_base.h"


#ifndef __NERD_H__
#define __NERD_H__

typedef struct Nerd {
    Sensor *sensor;
    KnowledgeBase *knowledge_base;
    unsigned int breadth, depth, epochs;
    float promotion_weight, demotion_weight;
    uint_fast8_t partial_observation;
} Nerd;

Nerd *nerd_constructor(const char * const filepath, const uint_fast8_t reuse,
const float activation_threshold, const unsigned int breadth, const unsigned int depth,
const unsigned int epochs, const float promotion_weight, const float demotion_weight,
const uint_fast8_t partial_observation);
void nerd_destructor(Nerd ** const nerd);
void nerd_start_learning(Nerd * const nerd);
void nerd_to_string(const Nerd * const nerd);
void nerd_to_file(const Nerd * const nerd, const char * const filepath);

#endif