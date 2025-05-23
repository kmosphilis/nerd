#include <pcg_variants.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "metrics.h"
#include "nerd_utils.h"

/**
 * @brief Evaluates whether Nerd's KnowledgeBase can predict all the observed
 * Literals when hidding a Literal. If the given observation has 6 Literals, 6
 * different scenarios will be tested accordingly.
 *
 * @param nerd The Nerd struct where the learnt KnowledgeBase to evaluate is.
 * @param inference_engine An inference engine function. First param should be
 * for the knowledge_base (Knolwedgebase *), second for the observation (Scene
 * *), and the last should be for the inference to be saved (Scene **).
 * @param sensor_to_evaluate The Sensor * containing the evaluation samples.
 * @param total_hidden A size_t pointer to save the total number of the Literals
 * that the algorithm has hidden.
 * @param total_recovered A size_t pointer to save the total number of correctly
 * recovered hidden Literals.
 * @param total_incorrectly_recovered A size_t pointer to save the total number
 * of incorrectly recovered hidden Literals (opposed Literals). If NULL, the
 * number will ne discarded.
 * @param total_not_recovered A size_t pointer to save the total number of
 * hidden Literals that were not recovered. If NULL, the number will be
 * discarded.
 *
 * @return 0 if the function was executed successfully, -1 if one of the given
 * parameters was NULL, -2 if a non existant path was given to
 * sensor_to_evaluate, or -3 if an error has occurred.
 */
int evaluate_all_literals(
    const Nerd *const nerd,
    void (*inference_engine)(const KnowledgeBase *const knowledge_base,
                             const Scene *const restrict observation,
                             Scene **inference),
    const Sensor *const sensor_to_evaluate, size_t *const restrict total_hidden,
    size_t *const restrict total_recovered,
    size_t *const restrict total_incorrectly_recovered,
    size_t *const restrict total_not_recovered) {
  if (!(nerd && sensor_to_evaluate && total_hidden && total_recovered)) {
    return -1;
  }

  const size_t total_observations =
      sensor_get_total_observations(sensor_to_evaluate);

  Literal *removed_literal;
  Scene *observation, *inference;

  char *expected_header = NULL;
  size_t j, total_hidden_ = 0, total_recovered_ = 0,
            total_incorrectly_recovered_ = 0, total_not_recovered_ = 0;
  unsigned int i, k;
  for (j = 0; j < total_observations; ++j) {
    sensor_get_next_scene(sensor_to_evaluate, &observation);

    total_hidden_ += observation->size;
    for (i = 0; i < observation->size; ++i) {
      scene_remove_literal(observation, 0, &removed_literal);

      inference_engine(nerd->knowledge_base, observation, &inference);

      if (sensor_to_evaluate->header) {
        expected_header = (char *)malloc(
            (strstr(removed_literal->atom, "_") - removed_literal->atom + 2) *
            sizeof(char));
      }
      for (k = 0; k < inference->size; ++k) {
        switch (literal_equals(removed_literal, inference->literals[k])) {
        case 1:
          ++total_recovered_;
          goto finished;
        case 0:
          if ((literal_opposed(removed_literal, inference->literals[k]) == 1) ||
              (expected_header &&
               (strstr(removed_literal->atom, expected_header) &&
                !removed_literal->sign))) {
            ++total_incorrectly_recovered_;
            goto finished;
          }
          break;
        default:
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

  return 0;
}

/**
 * @brief Evalaute whether the Nerd's learnt KnowledgeBase can predict (recover)
 * the random Literals that will be hidden (removed) by the algorithm.
 *
 * @param nerd The Nerd struct where the learnt KnowledgeBase to evaluate is.
 * @param inference_engine An inference engine function. First param should be
 * for the knowledge_base (Knolwedgebase *), second for the observation (Scene
 * *), and the last should be for the inference to be saved (Scene **).
 * @param sensor_to_evaluate The Sensor * containing the evaluation samples.
 * @param ratio A float variable which indicates the ratio of the Literals to be
 * hidden.
 * @param total_hidden A size_t pointer to save the total number of the Literals
 * that the algorithm has hidden.
 * @param total_recovered A size_t pointer to save the total number of correctly
 * recovered hidden Literals.
 * @param total_incorrectly_recovered A size_t pointer to save the total number
 * of incorrectly recovered hidden Literals (opposed Literals). If NULL, the
 * number will ne discarded.
 * @param total_not_recovered A size_t pointer to save the total number of
 * hidden Literals that were not recovered. If NULL, the number will be
 * discarded.
 *
 * @return 0 if the function was executed successfully, -1 if one of the given
 * parameters was NULL or the ratio was not in the range (0, 1), or -2 if a non
 * existant path was given to sensor_to_evaluate.
 */
int evaluate_random_literals(
    const Nerd *const nerd,
    void (*inference_engine)(const KnowledgeBase *const knowledge_base,
                             const Scene *const restrict observation,
                             Scene **inference),
    const Sensor *const sensor_to_evaluate, const float ratio,
    size_t *const restrict total_hidden, size_t *const restrict total_recovered,
    size_t *const restrict total_incorrectly_recovered,
    size_t *const restrict total_not_recovered) {
  if (!(nerd && sensor_to_evaluate && total_hidden && total_recovered &&
        (ratio > 0) && (ratio < 1))) {
    return -1;
  }

  const size_t total_observations =
      sensor_get_total_observations(sensor_to_evaluate);

  Literal *removed_literal = NULL;
  Scene *removed_literals = NULL, *observation = NULL, *inference = NULL;
  pcg32_random_t *rng = global_rng;
  if (!rng) {
    rng = (pcg32_random_t *)malloc(sizeof(pcg32_random_t));
    pcg32_srandom_r(rng, time(NULL), 314159U);
  }

  int equals_result;
  size_t total_hidden_ = 0, total_recovered_ = 0,
         total_incorrectly_recovered_ = 0, total_not_recovered_ = 0, remaining,
         new_size;
  unsigned int i, j, k, *possible_indices = NULL;
  char *expected_header = NULL;

  for (i = 0; i < total_observations; ++i) {
    removed_literals = scene_constructor(true);
    sensor_get_next_scene(sensor_to_evaluate, &observation);
    const size_t observation_size = observation->size;

    possible_indices = (unsigned int *)malloc(observation_size * sizeof(int));
    for (j = 0; j < observation_size; ++j) {
      possible_indices[j] = j;
    }

    new_size = observation_size - (observation_size * ratio);
    remaining = observation->size;

    while (removed_literals->size != new_size) {
      scene_remove_literal(observation,
                           possible_indices[pcg32_random_r(rng) % remaining--],
                           &removed_literal);
      scene_add_literal(removed_literals, &removed_literal);
    }
    safe_free(possible_indices);

    inference_engine(nerd->knowledge_base, observation, &inference);

    total_hidden_ += removed_literals->size;

    for (j = 0; j < removed_literals->size; ++j) {
      removed_literal = removed_literals->literals[j];
      size_t header_size = 0;
      if (sensor_to_evaluate->header) {
        header_size =
            strchr(removed_literal->atom, '_') - removed_literal->atom + 2;
        expected_header = (char *)malloc(header_size * sizeof(char));
        strncpy(expected_header, removed_literal->atom, header_size - 1);
        expected_header[header_size - 1] = '\0';
      }
      for (k = 0; k < inference->size; ++k) {
        equals_result = literal_equals(removed_literal, inference->literals[k]);
        if (equals_result == 1) {
          ++total_recovered_;
          goto next_literal;
        } else if (equals_result == 0) {
          if ((literal_opposed(removed_literal, inference->literals[k]) == 1) ||
              (sensor_to_evaluate->header &&
               (strstr(inference->literals[k]->atom, expected_header) &&
                !removed_literal->sign))) {
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

  if (!global_rng) {
    free(rng);
  }

  *total_hidden = total_hidden_;
  *total_recovered = total_recovered_;

  if (total_incorrectly_recovered) {
    *total_incorrectly_recovered = total_incorrectly_recovered_;
  }

  if (total_not_recovered) {
    *total_not_recovered = total_not_recovered_;
  }

  return 0;
}

/**
 * @brief Evaluates the Nerd's learnt KnowledgeBase by checking whether it can
 * find the corresponding Literal marked as a label. The labels are given as a
 * Context with the labels. The algorithm will find the label from the original
 * observation, and then it will use the integrated inference engine to find out
 * whether the engine can infer that Literal or not.
 *
 * @param nerd The Nerd struct where the learnt KnowledgeBase to evaluate is.
 * @param inference_engine An inference engine function returning int. First
 * param should be for the knowledge_base (Knolwedgebase *), second for the
 * total observations, third for the observations (Scene **), third should be
 * for the inferences to be saved (Scene ***), and the last one should be used
 * to save the inferring rules as a strings separated with a new line '\n' (char
 * **).
 * @param sensor_to_evaluate The Sensor * containing the evaluation samples.
 * @param labels The Context containing all the Literals that act a labels.
 * @param accuracy A pointer to a float variable to save the overall accuracy of
 * the KnowledgeBase over the given samples. If NULL is given, it will not be
 * saved.
 * @param abstain_ratio A pointer to a float variable to save the abstain ratio
 * of the KnowledgeBase over the given samples. Abstain means that with the
 * given KnowledgeBase a label cannot be predicted, either correct or incorrect.
 * If NULL is given, this ratio will not be saved.
 * @param total_observations A size_t * to save the number of total
 * observations. If NULL, it will not be saved.
 * @param observations A Scene *** to save the observations. It should be a
 * reference to a Scene **. Requires total_observations to work. If NULL is
 * given, they will not be saved.
 * @param inferences A Scene *** to save the inferences. It should be a
 * reference to a Scene **. Requires total_observations to work. If NULL is
 * given, they will not be saved.
 * @param save_inferring_rules A char ** (reference to a char *) to save the
 * inferring rules as a string. If NULL, they won't be saved.
 * @param partial_observation Indicates if the observation is partially
 * observed, and the label could be missing.
 *
 * @return 0 if the evaluation ended successfully, -1 if it one nerd, settings,
 * file_to_evaluation or labels where NULL, > 0 which will be the index of the
 * first observation that does not have a provided label (index calculated from
 * the given file), -2 if total_observation was not given, but either
 * observations or inferences were given.
 */
int evaluate_labels(
    const Nerd *const nerd,
    int (*inference_engine_batch)(const KnowledgeBase *const knowledge_base,
                                  const size_t total_observations,
                                  Scene **restrict observation,
                                  Scene ***const inference,
                                  char **const save_inferring_rules),
    const Sensor *const sensor_to_evaluate, const Context *const labels,
    float *const restrict accuracy, float *const restrict abstain_ratio,
    size_t *const total_observations, Scene ***const restrict observations,
    Scene ***const restrict inferences, char **const save_inferring_rules,
    bool partial_observation) {
  if (!(nerd && sensor_to_evaluate && labels)) {
    return -1;
  }

  if ((observations || inferences) && !total_observations) {
    return -2;
  }

  const size_t _total_observations =
      sensor_get_total_observations(sensor_to_evaluate);

  if (total_observations) {
    *total_observations = _total_observations;
  }

  Scene **_observations =
            (Scene **)malloc(sizeof(Scene *) * _total_observations),
        **_inferences = NULL;
  unsigned int *evaluation_literal_indices =
      (unsigned int *)malloc(_total_observations * sizeof(int));
  int label_index;

  unsigned int i, j;
  for (i = 0; i < _total_observations; ++i) {
    sensor_get_next_scene(sensor_to_evaluate, &(_observations[i]));
    for (j = 0; j < labels->size; ++j) {
      if ((label_index = scene_literal_index(_observations[i],
                                             labels->literals[j])) > -1) {
        evaluation_literal_indices[i] = j;
        scene_remove_literal(_observations[i], label_index, NULL);
        break;
      }
    }

    if (label_index < 0) {
      if (!partial_observation) {
        unsigned int temp;
        for (temp = 0; temp <= i; ++temp) {
          scene_destructor(&(_observations[temp]));
        }
        free(_observations);
        free(evaluation_literal_indices);
        return i + 1;
      }
    }
  }

  inference_engine_batch(nerd->knowledge_base, _total_observations,
                         _observations, &_inferences, save_inferring_rules);

  if (accuracy || abstain_ratio) {
    unsigned int positives = 0, negatives = 0, unobserved = 0;
    bool found = false;
    int current_index;
    for (i = 0; i < _total_observations; ++i) {
      for (j = 0; j < labels->size; ++j) {
        current_index =
            scene_literal_index(_inferences[i], labels->literals[j]);
        if (current_index > -1) {
          if (evaluation_literal_indices[i] == j) {
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

      found = false;
    }

    if (accuracy) {
      *accuracy = ((float)positives) / _total_observations;
    }

    if (abstain_ratio) {
      *abstain_ratio = ((float)unobserved) / _total_observations;
    }
  }

  if (observations) {
    *observations = _observations;
  } else {
    for (i = 0; i < _total_observations; ++i) {
      scene_destructor(&(_observations[i]));
    }
    free(_observations);
  }

  if (inferences) {
    *inferences = _inferences;
  } else {
    for (i = 0; i < _total_observations; ++i) {
      scene_destructor(&(_inferences[i]));
    }
    free(_inferences);
  }

  free(evaluation_literal_indices);

  return 0;
}
