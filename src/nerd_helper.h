#ifndef NERD_HELPER_H
#define NERD_HELPER_H

#define BUFFER_SIZE 256

#include "knowledge_base.h"
#include "scene.h"

typedef struct PrudensSettings *PrudensSettings_ptr;

extern PrudensSettings_ptr global_prudens_settings;

int prudensjs_settings_constructor(const char *const argv0,
                                   const char *const test_directory,
                                   const char *const constraints_file,
                                   char *extra_args);
int prudensjs_settings_destructor();

void prudensjs_inference(const KnowledgeBase *const knowledge_base,
                         const Scene *const restrict observation,
                         Scene **const inference);
int prudensjs_inference_batch(const KnowledgeBase *const knowledge_base,
                              const size_t observation_size,
                              Scene **restrict observations,
                              Scene ***const inferences,
                              char **const save_inferring_rules);

#endif
