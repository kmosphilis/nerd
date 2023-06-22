#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pcg_variants.h>

#include "nerd_utils.h"
#include "metrics.h"

/**
 * @brief Evaluates whether Nerd's KnowledgeBase can predict all the observed Literals when hidding
 * a Literal. If the given observation has 6 Literals, 6 different scenarios will be tested
 * accordingly.
 *
 * @param nerd The Nerd struct where the learnt KnowledgeBase to evaluate is.
 * @param settings A PrudensSettings_ptr which has all the necessary options for Prudens JS to run.
 * @param scene The scene to evaluate the learnt KnowledgeBase with.
 * @param total_hidden A size_t pointer to save the total number of the Literals that the algorithm
 * has hidden.
 * @param total_recovered A size_t pointer to save the total number of correctly recovered hidden
 * Literals.
 * @param total_incorrectly_recovered A size_t pointer to save the total number of incorrectly
 * recovered hidden Literals (opposed Literals). If NULL, the number will ne discarded.
 * @param total_not_recovered A size_t pointer to save the total number of hidden Literals that were
 * not recovered. If NULL, the number will be discarded.
 *
 * @return 0 if the function was executed successfully, -1 if one of the given parameters was NULL,
 * -2 if a non existant path was given to file_to_evaluate, or -3 if an error has occurred.
*/
int evaluate_all_literals(const Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const file_to_evaluate, size_t * const restrict total_hidden,
size_t * const restrict total_recovered, size_t * const restrict total_incorrectly_recovered,
size_t * const restrict total_not_recovered) {
    if (!(nerd && settings && file_to_evaluate && total_hidden && total_recovered)) {
        return -1;
    }

    Sensor *evaluation_sensor = sensor_constructor_from_file(file_to_evaluate,
    nerd->sensor->delimiter, false, nerd->sensor->header != NULL);

    if (!evaluation_sensor) {
        return -2;
    }

    const size_t total_observations = sensor_get_total_observations(evaluation_sensor);

    Literal *removed_literal;
    Scene *observation, *inference;

    char *expected_header = NULL;
    size_t j, total_hidden_ = 0, total_recovered_ = 0, total_incorrectly_recovered_ = 0,
    total_not_recovered_ = 0;
    unsigned int i, k;
    for (j = 0; j < total_observations; ++j) {
        sensor_get_next_scene(evaluation_sensor, &observation, false, NULL);

        total_hidden_ += observation->size;
        for (i = 0; i < observation->size; ++i) {
            scene_remove_literal(observation, 0, &removed_literal);

            prudensjs_inference(settings, nerd->knowledge_base, observation, &inference);

            if (evaluation_sensor->header) {
                expected_header = (char *) malloc((strstr(removed_literal->atom, "_")
                - removed_literal->atom + 2) * sizeof(char));
            }
            for (k = 0; k < inference->size; ++k) {
                switch (literal_equals(removed_literal, inference->literals[k])) {
                    case 1:
                        ++total_recovered_;
                        goto finished;
                    case 0:
                        if ((literal_opposed(removed_literal, inference->literals[k]) == 1) ||
                        (expected_header &&
                        (strstr(removed_literal->atom, expected_header) && !removed_literal->sign))) {
                            ++total_incorrectly_recovered_;
                            goto finished;
                        }
                        break;
                    default:
                        sensor_destructor(&evaluation_sensor);
                        scene_destructor(&observation);
                        scene_destructor(&inference);
                        safe_free(expected_header);
                        return -3;
                }
            }
            ++total_not_recovered_;
finished:
            safe_free(expected_header);
            scene_destructor(&inference);
            scene_add_literal(observation, &removed_literal);
        }

        scene_destructor(&observation);
    }

    *total_hidden = total_hidden_;
    *total_recovered = total_recovered_;

    if (total_incorrectly_recovered) {
        *total_incorrectly_recovered = total_incorrectly_recovered_;
    }

    if (total_not_recovered) {
        *total_not_recovered = total_not_recovered_;
    }

    sensor_destructor(&evaluation_sensor);

    return 0;
}

/**
 * @brief Evalaute whether the Nerd's learnt KnowledgeBase can predict (recover) the random Literals
 * that will be hidden (removed) by the algorithm.
 *
 * @param nerd The Nerd struct where the learnt KnowledgeBase to evaluate is.
 * @param settings A PrudensSettings_ptr which has all the necessary options for Prudens JS to run.
 * @param file_to_evaluate The filepath containing the evaluation samples.
 * @param ratio A float variable which indicates the ratio of the Literals to be hidden.
 * @param total_hidden A size_t pointer to save the total number of the Literals that the algorithm
 * has hidden.
 * @param total_recovered A size_t pointer to save the total number of correctly recovered hidden
 * Literals.
 * @param total_incorrectly_recovered A size_t pointer to save the total number of incorrectly
 * recovered hidden Literals (opposed Literals). If NULL, the number will ne discarded.
 * @param total_not_recovered A size_t pointer to save the total number of hidden Literals that were
 * not recovered. If NULL, the number will be discarded.
 *
 * @return 0 if the function was executed successfully, -1 if one of the given parameters was NULL
 * or the ratio was not in the range (0, 1), or -2 if a non existant path was given to
 * file_to_evaluate.
*/
int evaluate_random_literals(const Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const file_to_evaluate, const float ratio, size_t * const restrict total_hidden,
size_t * const restrict total_recovered, size_t * const restrict total_incorrectly_recovered,
size_t * const restrict total_not_recovered) {
    if (!(nerd && file_to_evaluate && settings && total_hidden && total_recovered
    && (ratio > 0) && (ratio < 1)
    )) {
        return -1;
    }

    Sensor *evaluation_sensor = sensor_constructor_from_file(file_to_evaluate,
    nerd->sensor->delimiter, false, nerd->sensor->header != NULL);

    if (!evaluation_sensor) {
        return -2;
    }

    const size_t total_observations = sensor_get_total_observations(evaluation_sensor);

    Literal *removed_literal = NULL;
    Scene *removed_literals = NULL, *observation = NULL, *inference = NULL;
    pcg32_random_t seed;
    if (global_seed) {
        seed = *global_seed;
    } else {
        pcg32_srandom_r(&seed, time(NULL), 314159U);
    }

    int equals_result;
    size_t total_hidden_ = 0, total_recovered_ = 0, total_incorrectly_recovered_ = 0,
    total_not_recovered_ = 0, remaining, new_size;
    unsigned int i, j, k, *possible_indices = NULL;
    char *expected_header = NULL;

    for (i = 0; i < total_observations; ++i) {
        removed_literals = scene_constructor(true);
        sensor_get_next_scene(evaluation_sensor, &observation, false, NULL);
        const size_t observation_size = observation->size;

        possible_indices = (unsigned int *) malloc(observation_size * sizeof(int));
        for (j = 0; j < observation_size; ++j) {
            possible_indices[j] = j;
        }

        new_size = observation_size - (observation_size * ratio);
        remaining = observation->size;

        while (removed_literals->size != new_size) {
            scene_remove_literal(observation, possible_indices[pcg32_random_r(&seed) % remaining--],
            &removed_literal);
            scene_add_literal(removed_literals, &removed_literal);
        }
        safe_free(possible_indices);

        prudensjs_inference(settings, nerd->knowledge_base, observation, &inference);

        total_hidden_ += removed_literals->size;

        for (j = 0; j < removed_literals->size; ++j) {
            removed_literal = removed_literals->literals[j];
            size_t header_size = 0;
            if (evaluation_sensor->header) {
                header_size = strchr(removed_literal->atom, '_') - removed_literal->atom + 2;
                expected_header = (char *) malloc(header_size * sizeof(char));
                strncpy(expected_header, removed_literal->atom, header_size - 1);
                expected_header[header_size - 1] = '\0';
            }
            for (k = 0; k < inference->size; ++k) {
                equals_result =
                literal_equals(removed_literal, inference->literals[k]);
                if (equals_result == 1) {
                    ++total_recovered_;
                    goto next_literal;
                } else if (equals_result == 0) {
                    if ((literal_opposed(removed_literal, inference->literals[k]) == 1) ||
                    (evaluation_sensor->header &&
                    (strstr(inference->literals[k]->atom, expected_header) && !removed_literal->sign))) {
                        ++total_incorrectly_recovered_;
                        goto next_literal;
                    }
                }
            }
            ++total_not_recovered_;
next_literal:
            safe_free(expected_header);
        }

        scene_destructor(&observation);
        scene_destructor(&removed_literals);
        scene_destructor(&inference);
    }

    *total_hidden = total_hidden_;
    *total_recovered = total_recovered_;

    if (total_incorrectly_recovered) {
        *total_incorrectly_recovered = total_incorrectly_recovered_;
    }

    if (total_not_recovered) {
        *total_not_recovered = total_not_recovered_;
    }

    sensor_destructor(&evaluation_sensor);

    return 0;
}

/**
 * @brief Evaluates the Nerd's learnt KnowledgeBase by checking whether it can find the
 * corresponding Literal marked as a label. The labels are given as a Context with the labels. The
 * algorithm will find the label from the original observation, and then it will use the integrated
 * inference engine to find out whether the engine can infer that Literal or not.
 *
 * @param nerd The Nerd struct where the learnt KnowledgeBase to evaluate is.
 * @param settings A PrudensSettings_ptr which has all the necessary options for Prudens JS to run.
 * @param file_to_evaluate The filepath containing the evaluation samples.
 * @param labels The Context containing all the Literals that act a labels.
 * @param accuracy A pointer to a float variable to save the overall accuracy of the KnowledgeBase
 * over the given samples.
 * @param abstain_ratio A pointer to a float variable to save the abstain ratio of the KnowledgeBase
 * over the given samples. Abstain means that with the given KnowledgeBase a label cannot be
 * predicted, either correct or incorrect. If NULL is given, this ratio will not be saved.
 *
 * @return 0 if the evaluation ended successfully, -1 if it one of the parameters was NULL (expect
 * abstain_ratio), > 0 which will be the index of the first observation that does not have a
 * provided label (index calculated from the given file).
*/
int evaluate_labels(const Nerd * const nerd, const PrudensSettings_ptr settings,
const char * const file_to_evaluate, const Context * const labels, float * const restrict accuracy,
float * const restrict abstain_ratio) {
    if (!(nerd && settings && file_to_evaluate && labels && accuracy)) {
        return -1;
    }

    Sensor *evaluation_sensor = sensor_constructor_from_file(file_to_evaluate,
    nerd->sensor->delimiter, false, nerd->sensor->header != NULL);

    if (!evaluation_sensor) {
        return -2;
    }

    const size_t total_observations = sensor_get_total_observations(evaluation_sensor);

    Scene *observation = NULL, *inference = NULL;
    unsigned int positives = 0, negatives = 0, unobserved = 0;
    bool found = false;

    int current_index, label_index;
    unsigned int j, i, evaluation_literal_index;
    for (j = 0; j < total_observations; ++j) {
        sensor_get_next_scene(evaluation_sensor, &observation, false, NULL);
        for (i = 0; i < labels->size; ++i) {
            if ((label_index = scene_literal_index(observation, labels->literals[i]))
            > -1) {
                evaluation_literal_index = i;
                scene_remove_literal(observation, label_index, NULL);
                break;
            }
        }

        if (label_index < 0) {
            scene_destructor(&observation);
            sensor_destructor(&evaluation_sensor);
            return j + 1;
        }

        prudensjs_inference(settings, nerd->knowledge_base, observation, &inference);

        for (i = 0; i < labels->size; ++i) {
            current_index = scene_literal_index(inference, labels->literals[i]);
            if (current_index > -1) {
                if (evaluation_literal_index == i) {
                    ++positives;
                } else {
                    ++negatives;
                }
                found = true;
                break;
            }
        }

        if (!found) {
            ++unobserved;
        }

        scene_destructor(&observation);
        scene_destructor(&inference);
    }

    sensor_destructor(&evaluation_sensor);

    *accuracy = ((float) positives) / total_observations;

    if (abstain_ratio) {
        *abstain_ratio = ((float) unobserved) / total_observations;
    }

    return 0;
}
