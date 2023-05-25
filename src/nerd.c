#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "nerd.h"
#include "metrics.h"

/**
 * @brief Constructs a Nerd structure (object).
 *
 * @param filepath The path to the file containing the learning instances for the Nerd algorithm.
 * @param delimiter The delimiter which separates each observation in the given file.
 * @param reuse Indicates whether to reuse the stream from the beginning when it reaches the EOF.
 * Use true (> 0) to indicate yes, and false (0) to indicate no. In the case that it should not
 * reuse the file, the algorithm will ignore the given epochs.
 * @param header Indicates whether the file contains a header or no. Use true (> 0) for yes, and
 * false (0) for no.
 * @param activation_threshold The threshold which determines whether a rule should get activated
 * or deactivated.
 * @param breadth The maximum size of Literals that a Rule should contain. //FIXME What if zero is given?
 * @param depth The maximum size of the KnowledgeBase //FIXME Currently not being used.
 * @param epochs The number of epochs the algorithm should learn for.
 * @param promotion_weight The amount that a Rule should be promoted with. It should be > 0.
 * @param demotion_weight The amount that a Rule should be demoted with. It should be > 0.
 * @param use_backward_chaining A boolean value which indicates whether the hypergraph should demoted
 * rules using the backward chaining algorithm or not.
 * @param partial_observation If partial_observation is > 0, the initial observation will be saved
 * here. If NULL is given, it will not be saved.
 *
 * @return A new Nerd object *. Use nerd_destructor to deallocate.
 */
Nerd *nerd_constructor(const char * const filepath, const char delimiter, const bool reuse,
const bool header, const float activation_threshold, const unsigned int breadth,
const unsigned int depth, const unsigned int epochs, const float promotion_weight,
const float demotion_weight, const bool use_backward_chaining, const bool partial_observation) {
    if (filepath) {
        Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));
        if ((nerd->sensor = sensor_constructor_from_file(filepath, delimiter, reuse, header))) {
            nerd->knowledge_base =
            knowledge_base_constructor(activation_threshold, use_backward_chaining);
            nerd->breadth = breadth;
            nerd->depth = depth;
            if (reuse) {
                nerd->epochs = epochs;
            } else {
                nerd->epochs = 1;
            }
            nerd->promotion_weight = promotion_weight;
            nerd->demotion_weight = demotion_weight;
            nerd->partial_observation = partial_observation;
            return nerd;
        }
        free(nerd);
    }
    return NULL;
}

/**
 * @brief Constructs a Nerd structure (object) using an existing nerd file. If the file has an
 * incorrect format or there are missing information, the process will fail.
 *
 * @param filepath The path to the file that contains previous a Nerd structure parameters (except
 * the number of epochs) and the learnt KnowledgeBase.
 * @param epochs The number of epochs the algorithm should learn for.
 * @param use_backward_chaining A boolean value which indicates whether the hypergraph should demoted
 * rules using the backward chaining algorithm or not.
 *
 * @return A new Nerd object * from the given filepath. Use nerd_destructor to deallocate.
*/
Nerd *nerd_constructor_from_file(const char * const filepath, const unsigned int epochs,
const bool use_backward_chaining) {
    if (filepath) {
        FILE *file = fopen(filepath, "rb");
        if (!file) {
            return NULL;
        }
        Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));
        nerd->knowledge_base = NULL;
        nerd->sensor = NULL;

        size_t buffer_size = BUFFER_SIZE;
        char *buffer = (char *) malloc(buffer_size * sizeof(char)), sensor_delimiter;
        unsigned short partial_observation, sensor_reuse, sensor_header;
        float activation_threshold;

        if (fscanf(file, "breadth: %zu\n", &(nerd->breadth)) != 1) {
            goto failed1;
        }
        if (fscanf(file, "depth: %zu\n", &(nerd->depth)) != 1) {
            goto failed1;
        }
        if (fscanf(file, "promotion_weight: %f\n", &(nerd->promotion_weight)) != 1) {
            goto failed1;
        }
        if (fscanf(file, "demotion_weight: %f\n", &(nerd->demotion_weight)) != 1) {
            goto failed1;
        }
        if (fscanf(file, "partial_observation: %hu\n", &partial_observation) != 1) {
            goto failed1;
        }
        nerd->partial_observation = partial_observation;
        if (fscanf(file, "sensor: %s '%c' %hu %hu\n", buffer, &sensor_delimiter, &sensor_reuse,
        &sensor_header) != 4) {
            goto failed1;
        }
        nerd->sensor = sensor_constructor_from_file(buffer, sensor_delimiter, sensor_reuse,
        sensor_header);

        if (nerd->sensor->reuse) {
            nerd->epochs = epochs;
        } else {
            nerd->epochs = 1;
        }

        long int previous_position = ftell(file);
        fscanf(file, "knowledge_base:\n");
        if (previous_position == ftell(file)) {
            goto failed2;
        }
        if (fscanf(file, "activation_threshold: %f\n", &activation_threshold) != 1) {
            goto failed2;
        }

        previous_position = ftell(file);
        fscanf(file, "rules:\n");
        if (previous_position == ftell(file)) {
            goto failed2;
        }

        free(buffer);
        nerd->knowledge_base =
        knowledge_base_constructor(activation_threshold, use_backward_chaining);

        fpos_t position;
        fgetpos(file, &position);
        char *tokens;
        Literal *literal;
        Rule *rule;
        Body *body = scene_constructor(true);

        buffer = (char *) malloc(buffer_size * sizeof(char));
        memset(buffer, 0, buffer_size);


        while(fgets(buffer, buffer_size, file) != NULL) {
            if (strlen(buffer) == buffer_size) {
                buffer_size <<= 1;
                buffer = realloc(buffer, buffer_size * sizeof(char));
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
            fgetpos(file, &position);
        }

        scene_destructor(&body);

        free(buffer);

        fclose(file);
        return nerd;
failed2:
    sensor_destructor(&(nerd->sensor));
failed1:
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
        (*nerd)->epochs = 0;
        (*nerd)->promotion_weight = INFINITY;
        (*nerd)->demotion_weight = INFINITY;
        sensor_destructor(&((*nerd)->sensor));
        knowledge_base_destructor(&((*nerd)->knowledge_base));
        (*nerd)->partial_observation = 0;
        free(*nerd);
        *nerd = NULL;
    }
}

/**
 * @brief Calls prudens-js using node-js. It create a file with a converted KnowledgeBase and a
 * Scene/Context, which holds an observation and saves the inferred Literals.
 *
 * @param knowledge_base The KnowledgeBase to be used in prudens-js.
 * @param observation A Scene/Context, which includes all the observed Literals.
 * @param inferred A Scene to save the inferred Literals by prudens-js. Deallocate using
 * scene_destructor.
*/
void prudensjs_inference(const KnowledgeBase * const knowledge_base,
const Scene * const restrict observation, Scene ** const inferred) {
    const char * const temp_filename = ".temp";
    char *knowledge_base_prudensjs = knowledge_base_to_prudensjs(knowledge_base),
    *context_prudensjs = context_to_prudensjs(observation);
    if (!(knowledge_base_prudensjs && context_prudensjs)) {
        return;
    }

    FILE *file = fopen(temp_filename, "wb");

    fprintf(file, "%s\n%s", knowledge_base_prudensjs, context_prudensjs);
    free(knowledge_base_prudensjs);
    free(context_prudensjs);
    fclose(file);
    system("node prudens-infer.js");

    file = fopen(temp_filename, "rb");
    if (feof(file)) {
        fclose(file);
        return;
    }

    char buffer[BUFFER_SIZE];
    *inferred = scene_constructor(true);
    Literal *literal;
    while (fscanf(file, "%s", buffer) != EOF) {
        literal = literal_constructor_from_string(buffer);
        scene_add_literal(*inferred, &literal);
    }

    fclose(file);
    remove(temp_filename);
}

/**
 * @brief Initiates the learning.
 *
 * @param nerd The Nerd structure containing all the info for learn new Rules.
 */
void nerd_start_learning(Nerd * const nerd) {
    if (!nerd) {
        return;
    }

    Scene *observation = NULL, *inferred = NULL, *uncovered = NULL;
    size_t epoch, iteration;
    const size_t total_observations = sensor_get_total_observations(nerd->sensor);

    struct timespec start, end, prudens_start_time, prudens_end_time;
    double prudens_total_time = 0;
    timespec_get(&start, TIME_UTC);

    for (epoch = 0; epoch < nerd->epochs; ++epoch) {
        for (iteration = 0; iteration < total_observations; ++iteration) {
            printf("\nEpoch %zu of %zu, Iteration %zu of %zu\n", epoch + 1, nerd->epochs,
            iteration + 1, total_observations);

            sensor_get_next_scene(nerd->sensor, &observation, nerd->partial_observation, NULL);
            timespec_get(&prudens_start_time, TIME_UTC);
            prudensjs_inference(nerd->knowledge_base, observation, &inferred);
            timespec_get(&prudens_end_time, TIME_UTC);

            prudens_total_time += (prudens_end_time.tv_sec - prudens_start_time.tv_sec) * 1e3 +
            (prudens_end_time.tv_nsec - prudens_start_time.tv_nsec) / 1e6;

            knowledge_base_create_new_rules(nerd->knowledge_base, observation, inferred,
            nerd->breadth, 5);

            scene_difference(observation, inferred, &uncovered);

            rule_hypergraph_update_rules(nerd->knowledge_base, observation, inferred,
            nerd->promotion_weight, nerd->demotion_weight);

            scene_destructor(&uncovered);
            scene_destructor(&observation);
            scene_destructor(&inferred);
        }
    }

    timespec_get(&end, TIME_UTC);
    double total_time = (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) / 1e6;
    printf("Time spend on nerd: %f ms\n", total_time - prudens_total_time);
    printf("Time spent on prudens: %f ms\n", prudens_total_time);
    printf("Total time: %f ms\n", total_time);
}

/**
 * @brief Saves/Converts the Nerd structure to a file which all the parameters that were used and the learnt
 * KnowledgeBase are saved, except the number of epochs.
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

    fprintf(file, "breadth: %zu\n", nerd->breadth);
    fprintf(file, "depth: %zu\n", nerd->depth);
    fprintf(file, "promotion_weight: %f\n", nerd->promotion_weight);
    fprintf(file, "demotion_weight: %f\n", nerd->demotion_weight);
    fprintf(file, "partial_observation: %hu\n", nerd->partial_observation);
    fprintf(file, "sensor: %s '%c' %hu %hu\n", nerd->sensor->filepath, nerd->sensor->delimiter,
    nerd->sensor->reuse, nerd->sensor->header_size > 0);

    fprintf(file, "knowledge_base:\n");
    fprintf(file, "  activation_threshold: %f\n", nerd->knowledge_base->activation_threshold);
    fprintf(file, "  rules:\n");

    for (i = 0; i < nerd->knowledge_base->active->length; ++i) {
        str = rule_to_string(nerd->knowledge_base->active->rules[i]);
        fprintf(file, "    %s,\n", str);
        free(str);
    }

    RuleQueue *inactive_rules;
    rule_hypergraph_get_inactive_rules(nerd->knowledge_base, &inactive_rules);

    for (i = 0; i < inactive_rules->length; ++i) {
        str = rule_to_string(inactive_rules->rules[i]);
        fprintf(file, "    %s,\n", str);
        free(str);
    }
    rule_queue_destructor(&inactive_rules);

    fclose(file);
}
