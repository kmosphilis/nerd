#ifndef NERD_HELPER_H
#define NERD_HELPER_H

#define BUFFER_SIZE 256

#include "knowledge_base.h"
#include "scene.h"

typedef struct PrudensSettings *PrudensSettings_ptr;

int prudensjs_settings_constructor(PrudensSettings_ptr *settings, const char * const argv0,
const char * const test_directory, const char * const constraints_file, char *extra_args);
int prudensjs_settings_destructor(PrudensSettings_ptr *settings);

void prudensjs_inference(const PrudensSettings_ptr settings,
const KnowledgeBase * const knowledge_base, const Scene * const restrict observation,
Scene ** const inference);
int prudensjs_inference_batch(const PrudensSettings_ptr settings,
const KnowledgeBase * const knowledge_base, const size_t observation_size,
Scene ** restrict observations, Scene *** const inferences, char ** const save_inferring_rules);

#endif
