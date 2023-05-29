#ifndef NERD_H
#define NERD_H

#include <stdbool.h>

#include "sensor.h"
#include "knowledge_base.h"

// Optional parameter to be defined in the file with the main function if file will not be runned
// under the src/ directory.
extern char *nerd_current_directory;

// Optional parameter to be defined in the file with the main function to save the temporary file
// used for Prudens JS and the output of Nerd at each epoch.
extern char *test_directory;

// Optional parameter to be defined in the file with the main function which should contain the path
// of a file with defined incompatibility rules (Generalised Conflict Semantic rules) as defined
// under Prudens JS.
extern char *constraints_file;

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
const bool use_backward_chaining);
void nerd_destructor(Nerd ** const nerd);
void prudensjs_inference(const KnowledgeBase * const knowledge_base,
const Scene * const restrict observation, Scene ** const inferred);
void nerd_start_learning(Nerd * const nerd);
void nerd_to_string(const Nerd * const nerd);
void nerd_to_file(const Nerd * const nerd, const char * const filepath);

#endif
