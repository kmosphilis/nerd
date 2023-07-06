#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "nerd_utils.h"
#include "nerd.h"
#include "metrics.h"

#define EPOCH_FILE_NAME_FORMAT "%siteration-%zu-instance%zu.nd"
#define EVALUATION_FILE_NAME_FORMAT "%sepoch-%zu-results.txt"

#define SECONDS_TO_MILLISECONDS 1e3
#define NANOSECONDS_TO_MILLISECONDS 1e-6

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
 * @param use_backward_chaining A boolean value which indicates whether the hypergraph should
 * demoted rules using the backward chaining algorithm or not.
 * @param reuse_sensor A boolean value which indicates whether the sensor should be reused or not.
 *
 * @return A new Nerd object * from the given filepath. Use nerd_destructor to deallocate.
*/
Nerd *nerd_constructor_from_file(const char * const filepath, const unsigned int epochs,
const bool use_backward_chaining, const bool reuse_sensor) {
    if (filepath) {
        FILE *file = fopen(filepath, "rb");
        if (!file) {
            return NULL;
        }
        Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));
        nerd->sensor = NULL;

        size_t buffer_size = BUFFER_SIZE;
        char *buffer = (char *) calloc(buffer_size, sizeof(char)), sensor_delimiter;
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

        nerd->epochs = epochs;
        if (reuse_sensor) {
            if (fscanf(file, "sensor: %s '%c' %hu %hu\n", buffer, &sensor_delimiter, &sensor_reuse,
            &sensor_header) != 4) {
                goto failed1;
            }
            nerd->sensor = sensor_constructor_from_file(buffer, sensor_delimiter, sensor_reuse,
            sensor_header);

            if (nerd->sensor) {
                if (!nerd->sensor->reuse) {
                    nerd->epochs = 1;
                }
            } else {
                goto failed1;
            }
        } else {
            int c;
            while ((c = fgetc(file)) != '\n');
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
            memset(buffer, 0, strlen(buffer));
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
        safe_free(*nerd);
    }
}

/**
 * @brief Initiates the learning.
 *
 * @param nerd The Nerd structure containing all the info for learn new Rules.
 * @param settings A PrudensSettings_ptr which has all the necessary options for Prudens JS to run.
 * @param test_directory (Optional) The directory path to save the nerd at each instance.
 * @param evaluation_filepath (Optional) The path of the evaluation (test) dataset. If it is given,
 * the algorithm will perform an evaluation of the learnt KnowledgeBase at each iteration (using
 * evaluate_all_literals and evaluate_random_literals).
 * @param labels (Optional) A Context containing all the Literals that should be considered as
 * labels. If it is given, the algorithm will perform a label evaluation of the learnt KnowledgeBase
 * at each iteration (using evaluate_labels).
 */
void nerd_start_learning(Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const test_directory, const char * const evaluation_filepath,
const Context * const restrict labels) {
    if (!(nerd && settings)) {
        return;
    }

    Scene *observation = NULL, *inferred = NULL;
    size_t iteration, instance;
    const size_t total_observations = sensor_get_total_observations(nerd->sensor);

    struct timespec start, end, prudens_start_time, prudens_end_time, start_convert, end_convert;
    size_t prudens_total_time = 0, convert_total = 0;
    char *nerd_at_epoch_filename = NULL, *results_at_epoch_filename = NULL;
    timespec_get(&start, TIME_UTC);

    for (iteration = 0; iteration < nerd->epochs; ++iteration) {
        for (instance = 0; instance < total_observations; ++instance) {
            printf("\nIteration %zu of %zu, Instance %zu of %zu\n", iteration + 1, nerd->epochs,
            instance + 1, total_observations);

            sensor_get_next_scene(nerd->sensor, &observation, nerd->partial_observation, NULL);
            timespec_get(&prudens_start_time, TIME_UTC);
            prudensjs_inference(settings, nerd->knowledge_base, observation, &inferred);
            timespec_get(&prudens_end_time, TIME_UTC);

            prudens_total_time +=
            (prudens_end_time.tv_sec - prudens_start_time.tv_sec) * SECONDS_TO_MILLISECONDS
            + (prudens_end_time.tv_nsec - prudens_start_time.tv_nsec) * NANOSECONDS_TO_MILLISECONDS;

            knowledge_base_create_new_rules(nerd->knowledge_base, observation, inferred,
            nerd->breadth, 5, labels);

            rule_hypergraph_update_rules(nerd->knowledge_base, observation, inferred,
            nerd->promotion_weight, nerd->demotion_weight);

            scene_destructor(&observation);
            scene_destructor(&inferred);

            timespec_get(&start_convert, TIME_UTC);
            if (test_directory) {
                nerd_at_epoch_filename = (char *) malloc((snprintf(NULL, 0, EPOCH_FILE_NAME_FORMAT,
                test_directory, iteration + 1, instance + 1)  + 1) * sizeof(char));
                sprintf(nerd_at_epoch_filename, EPOCH_FILE_NAME_FORMAT, test_directory,
                iteration + 1, instance + 1);
                nerd_to_file(nerd, nerd_at_epoch_filename);
                safe_free(nerd_at_epoch_filename);
            }
            timespec_get(&end_convert, TIME_UTC);
            convert_total += (end_convert.tv_sec - start_convert.tv_sec) * SECONDS_TO_MILLISECONDS
            + (end_convert.tv_nsec - start_convert.tv_nsec) * NANOSECONDS_TO_MILLISECONDS;
        }

        timespec_get(&start_convert, TIME_UTC);
        if (evaluation_filepath) {
            const char *test_space = " ", *test_result_space = "  ";
            results_at_epoch_filename = (char *) malloc((snprintf(NULL, 0,
            EVALUATION_FILE_NAME_FORMAT, test_directory, iteration + 1) + 1) * sizeof(char));
            sprintf(results_at_epoch_filename, EVALUATION_FILE_NAME_FORMAT, test_directory,
            iteration + 1);
            FILE *results = fopen(results_at_epoch_filename, "wb");

            size_t total_hidden, total_recovered, total_not_recovered, total_incorrectly_recovered;
            int result;
            // result = evaluate_all_literals(nerd, settings, evaluation_filepath, &total_hidden,
            // &total_recovered, &total_incorrectly_recovered, &total_not_recovered);
            // fprintf(results, "Epoch %zu of %zu\n", epoch + 1, nerd->epochs);
            // fprintf(results, "%sEvaluating All Literals:\n", test_space);

            // if (result == 0) {
            //     fprintf(results, "%sh r i n\n", test_result_space);
            //     fprintf(results, "%s%zu %zu %zu %zu\n", test_result_space, total_hidden,
            //     total_recovered, total_incorrectly_recovered, total_not_recovered);
            // } else if (result == -2) {
            //     fprintf(results, "%s'%s' is not a valid evaluation_path\n", test_result_space,
            //     evaluation_filepath);
            // }

            const unsigned int ratios_size = 3;
            const float ratios[] = {0.2, 0.4, 0.6};

            unsigned int i;
            for (i = 0; i < ratios_size; ++i) {
                fprintf(results, "%sEvaluating Random Literals with ratio %.1f:\n", test_space,
                ratios[i]);

                result = evaluate_random_literals(nerd, settings, evaluation_filepath, ratios[i],
                &total_hidden, &total_recovered, &total_incorrectly_recovered,
                &total_not_recovered);

                if (result == 0) {
                    fprintf(results, "%sh r i n\n", test_result_space);
                    fprintf(results, "%s%zu %zu %zu %zu\n", test_result_space, total_hidden,
                    total_recovered, total_incorrectly_recovered, total_not_recovered);
                } else {
                    fprintf(results, "%s'%s' is not a valid evaluation_path\n", test_result_space,
                    evaluation_filepath);
                }
            }

            float accuracy, abstain_ratio;
            if (labels) {
                result = evaluate_labels(nerd, settings, evaluation_filepath, labels,
                nerd->sensor->delimiter, nerd->sensor->header != NULL, &accuracy, &abstain_ratio);
                if (result == 0) {
                    fprintf(results, "%sEvaluating Labels:\n", test_space);
                    fprintf(results, "%saccuracy abstain\n", test_result_space);
                    fprintf(results, "%s%.2f %.2f\n", test_result_space, accuracy, abstain_ratio);
                } else if (result > 0) {
                    fprintf(results, "%sNo label was given for line %u\n", test_result_space,
                    result);
                }
            }

        fclose(results);
        safe_free(results_at_epoch_filename);
        }

        timespec_get(&end_convert, TIME_UTC);
        convert_total += (end_convert.tv_sec - start_convert.tv_sec) * SECONDS_TO_MILLISECONDS
        + (end_convert.tv_nsec - start_convert.tv_nsec) * NANOSECONDS_TO_MILLISECONDS;
    }

    timespec_get(&end, TIME_UTC);
    size_t total_time = ((end.tv_sec - start.tv_sec) * SECONDS_TO_MILLISECONDS
    + (end.tv_nsec - start.tv_nsec) * NANOSECONDS_TO_MILLISECONDS) - convert_total;
    // printf("Evaluation time: %zu ms\n", convert_total);
    printf("Time spend on nerd: %zu ms\n", total_time - prudens_total_time);
    printf("Time spent on prudens: %zu ms\n", prudens_total_time);
    printf("Total time: %zu ms\n", total_time);
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
