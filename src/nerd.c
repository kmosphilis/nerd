#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "nerd_utils.h"
#include "nerd.h"
#include "metrics.h"

#define SECONDS_TO_MILLISECONDS 1e3
#define NANOSECONDS_TO_MILLISECONDS 1e-6

/**
 * @brief Constructs a Nerd structure (object).
 *
 * @param activation_threshold The threshold which determines whether a rule should get activated
 * or deactivated.
 * @param max_rules_per_instance The maximum Rules that should be learnt in an iteration. Created
 * Rules that exist will be discarded.
 * @param breadth The maximum size of Literals that a Rule should contain. //FIXME What if zero is given?
 * @param depth The maximum size of the KnowledgeBase //FIXME Currently not being used.
 * @param promotion_weight The amount that a Rule should be promoted with. It should be > 0.
 * @param demotion_weight The amount that a Rule should be demoted with. It should be > 0.
 * @param use_backward_chaining A boolean value which indicates whether the hypergraph should demote
 * rules using the backward chaining algorithm or not.
 * @param increasing_demotion A boolean value which indicates whether the chaining demotion should
 * be increasing or not. It only works if and only if backward chaining demotion is enabled.
 *
 * @return A new Nerd object *. Use nerd_destructor to deallocate.
 */
Nerd *nerd_constructor(const float activation_threshold, const unsigned int max_rules_per_instance,
const unsigned int breadth, const unsigned int depth, const float promotion_weight,
const float demotion_weight, const bool use_backward_chaining, const bool increasing_demotion) {
    Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));
    nerd->knowledge_base = knowledge_base_constructor(activation_threshold, use_backward_chaining);
    nerd->max_rules_per_instance = max_rules_per_instance;
    nerd->breadth = breadth;
    nerd->depth = depth;
    nerd->promotion_weight = promotion_weight;
    nerd->demotion_weight = demotion_weight;
    nerd->increasing_demotion = increasing_demotion;
    return nerd;
}

/**
 * @brief Constructs a Nerd structure (object) using an existing nerd file. If the file has an
 * incorrect format or there are missing information, the process will fail.
 *
 * @param filepath The path to the file that contains previous a Nerd structure parameters (except
 * the number of epochs) and the learnt KnowledgeBase.
 * @param use_backward_chaining A boolean value which indicates whether the hypergraph should
 * demoted rules using the backward chaining algorithm or not.
 *
 * @return A new Nerd object * from the given filepath. Use nerd_destructor to deallocate.
*/
Nerd *nerd_constructor_from_file(const char * const filepath, const bool use_backward_chaining) {
    if (filepath) {
        FILE *file = fopen(filepath, "rb");
        if (!file) {
            return NULL;
        }
        Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));

        size_t buffer_size = BUFFER_SIZE;
        char *buffer = (char *) calloc(buffer_size, sizeof(char));
        unsigned short increasing_demotion;
        float activation_threshold;

        if (fscanf(file, "max_rules_per_instance: %zu\n", &(nerd->max_rules_per_instance)) != 1) {
            goto failed;
        }
        if (fscanf(file, "breadth: %zu\n", &(nerd->breadth)) != 1) {
            goto failed;
        }
        if (fscanf(file, "depth: %zu\n", &(nerd->depth)) != 1) {
            goto failed;
        }
        if (fscanf(file, "promotion_weight: %f\n", &(nerd->promotion_weight)) != 1) {
            goto failed;
        }
        if (fscanf(file, "demotion_weight: %f %hu\n", &(nerd->demotion_weight),
        &increasing_demotion) != 2) {
            goto failed;
        }
        nerd->increasing_demotion = increasing_demotion;

        long int previous_position = ftell(file);
        fscanf(file, "knowledge_base:\n");
        if (previous_position == ftell(file)) {
            goto failed;
        }
        if (fscanf(file, "activation_threshold: %f\n", &activation_threshold) != 1) {
            goto failed;
        }

        previous_position = ftell(file);
        fscanf(file, "rules:\n");
        if (previous_position == ftell(file)) {
            goto failed;
        }

        nerd->knowledge_base =
        knowledge_base_constructor(activation_threshold, use_backward_chaining);

        fpos_t position;
        fgetpos(file, &position);
        char *tokens;
        Literal *literal;
        Rule *rule;
        Body *body = scene_constructor(true);

        memset(buffer, 0, strlen(buffer));

        while(fgets(buffer, buffer_size, file) != NULL) {
            if ((strlen(buffer) + 1) == buffer_size) {
                buffer_size <<= 1;
                buffer = realloc(buffer, buffer_size * sizeof(char));
                memset(buffer + (buffer_size >> 1), 0, buffer_size >> 1);
                fsetpos(file, &position);
                continue;
            }

            tokens = strtok(buffer, " (),\n");

            while (tokens != NULL) {
                if (strcmp(tokens, "=>") == 0) {
                    tokens = strtok(NULL, " (),\n");
                    literal = literal_constructor_from_string(tokens);

                    tokens = strtok(NULL, " (),\n");
                    float weight = atof(tokens);

                    rule = rule_constructor(body->size, body->literals, &literal, weight, true);
                    knowledge_base_add_rule(nerd->knowledge_base, &rule);

                    scene_destructor(&body);
                    body = scene_constructor(true);
                } else {
                    if (tokens[0] == '-') {
                        literal = literal_constructor(tokens + 1, 0);
                    } else {
                        literal = literal_constructor(tokens, 1);
                    }
                    scene_add_literal(body, &literal);
                }
                tokens = strtok(NULL, " (),\n");
            }
            memset(buffer, 0, strlen(buffer));
            fgetpos(file, &position);
        }

        scene_destructor(&body);

        free(buffer);

        fclose(file);
        return nerd;
failed:
        free(nerd);
        free(buffer);
        fclose(file);
    }
    return NULL;
}

/**
 * @brief Destructs a Nerd structure (object).
 *
 * @param nerd The Nerd structure to be destructed. It should be a reference to the object's
 * pointer.
 */
void nerd_destructor(Nerd ** const nerd) {
    if (nerd && (*nerd)) {
        (*nerd)->breadth = 0;
        (*nerd)->depth = 0;
        (*nerd)->promotion_weight = INFINITY;
        (*nerd)->demotion_weight = INFINITY;
        knowledge_base_destructor(&((*nerd)->knowledge_base));
        safe_free(*nerd);
    }
}

/**
 * @brief Initiates the learning.
 *
 * @param nerd The Nerd structure containing all the info for learn new Rules.
 * @param inference_engine An inference engine function. If NULL, it will not function as intented,
 * and it * will only create rules.
 * @param observation A Scene * containing the current observation (instance) to learn from.
 * @param labels (Optional) A Context containing all the Literals that should be considered as
 * labels.
 * @param nerd_time_taken (Optional) A size_t * to save the time NERD took to finish learning. It
 * does not include the time taken by the inference engine (Prudens JS).
 * @param ie_time_taken (Optional) A size_t * to save the time Prudens JS took to finish the
 * application of the learnt KnowledgeBase.
 * @param header (Optional) A char ** containing the header of a file to compare attributes. The
 * rest two optional parameters should be given together.
 * @param header_size (Optional) The size_t of the header.
 * @param incompatibilities (Optional) A Scene ** containing the incompatible literals corresponding
 * to each header. It should have the same size, even if no incompatible literals are given.
 */
void nerd_train(Nerd * const nerd, void (*inference_engine)
(const KnowledgeBase * const knowledge_base, const Scene * const restrict observation,
Scene **inference), const Scene * const restrict observation,
const Context * const restrict labels, size_t * const nerd_time_taken, size_t * const ie_time_taken,
char **header, const size_t header_size, Scene **incompatibilities) {
    if (!(nerd && observation)) {
        return;
    }

    Scene *inferred = NULL;
    struct timespec start, end, prudens_start_time, prudens_end_time;

    timespec_get(&start, TIME_UTC);
    timespec_get(&prudens_start_time, TIME_UTC);

    if (inference_engine) {
        inference_engine(nerd->knowledge_base, observation, &inferred);
    }

    timespec_get(&prudens_end_time, TIME_UTC);

    knowledge_base_create_new_rules(nerd->knowledge_base, observation, inferred,
    nerd->breadth, nerd->max_rules_per_instance, labels);
    rule_hypergraph_update_rules(nerd->knowledge_base, observation, inferred,
    nerd->promotion_weight, nerd->demotion_weight, nerd->increasing_demotion, header, header_size,
    incompatibilities);

    scene_destructor(&inferred);

    timespec_get(&end, TIME_UTC);

    size_t prudens_total_time =
    (prudens_end_time.tv_sec - prudens_start_time.tv_sec) * SECONDS_TO_MILLISECONDS
    + (prudens_end_time.tv_nsec - prudens_start_time.tv_nsec) * NANOSECONDS_TO_MILLISECONDS;

    if (nerd_time_taken) {
        *nerd_time_taken = ((end.tv_sec - start.tv_sec) * SECONDS_TO_MILLISECONDS
        + (end.tv_nsec - start.tv_nsec) * NANOSECONDS_TO_MILLISECONDS);
    }

    if (ie_time_taken) {
        *ie_time_taken = prudens_total_time;
    }
}

/**
 * @brief Saves/Converts the Nerd structure to a file which all the parameters that were used and
 * the learnt KnowledgeBase are saved, except the number of epochs.
 *
 * @param nerd The Nerd structure to be saved/converted to a file.
 * @param filepath The path and the name of the file which the Nerd structure will be saved to.
*/
void nerd_to_file(const Nerd * const nerd, const char * const filepath) {
    if (!(nerd && filepath)) {
        return;
    }

    FILE *file = fopen(filepath, "wb");
    unsigned int i;
    char *str = NULL;

    fprintf(file, "max_rules_per_instance: %zu\n", nerd->max_rules_per_instance);
    fprintf(file, "breadth: %zu\n", nerd->breadth);
    fprintf(file, "depth: %zu\n", nerd->depth);
    fprintf(file, "promotion_weight: %f\n", nerd->promotion_weight);
    fprintf(file, "demotion_weight: %f %hu\n", nerd->demotion_weight, nerd->increasing_demotion);

    fprintf(file, "knowledge_base:\n");
    fprintf(file, "  activation_threshold: %f\n", nerd->knowledge_base->activation_threshold);
    fprintf(file, "  rules:\n");

    for (i = 0; i < nerd->knowledge_base->active->length; ++i) {
        str = rule_to_string(nerd->knowledge_base->active->rules[i]);
        fprintf(file, "    %s,\n", str);
        safe_free(str);
    }

    RuleQueue *inactive_rules;
    rule_hypergraph_get_inactive_rules(nerd->knowledge_base, &inactive_rules);

    for (i = 0; i < inactive_rules->length; ++i) {
        str = rule_to_string(inactive_rules->rules[i]);
        fprintf(file, "    %s,\n", str);
        safe_free(str);
    }
    rule_queue_destructor(&inactive_rules);

    fclose(file);
}
