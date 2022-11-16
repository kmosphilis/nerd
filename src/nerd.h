#include "sensor.h"
#include "knowledge_base.h"


#ifndef __NERD_H__
#define __NERD_H__

typedef struct Nerd {
    Sensor sensor;
    KnowledgeBase knowledge_base;
    unsigned int breadth, depth, epochs;
    float promotion_weight, demotion_weight;
} Nerd;

void nerd_constructor(Nerd * const nerd, const char * const filepath, unsigned short reuse,
const float activation_threshold, const unsigned int breadth, const unsigned int depth,
const unsigned int epochs, const float promotion_weight, const float demotion_weight);
void nerd_destructor(Nerd * const nerd);
void nerd_start_learning(Nerd * const nerd);
void nerd_to_string(const Nerd * const nerd);
void nerd_to_file(const Nerd * const nerd, const char * const filepath);

#endif