#include <ctype.h>
#include <math.h>
#include <pcg_variants.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <argp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "metrics.h"
#include "nerd.h"
#include "nerd_helper.h"

#define DECIMAL_BASE 10
#define STATE_SEED 31415926535U
#define SEQUENCE_SEED 27182818284U

#define TRAIN ".train_set"
#define TEST ".test_set"
#define TEST_DIRECTORY "timestamp=%zu_test=%d/"
#define INFO_FILE_NAME "info"

int close_dataset_and_exit(FILE *dataset) {
  if (dataset) {
    fclose(dataset);
  }
  return EXIT_FAILURE;
}

typedef struct Arguments {
  char *dataset_path, *labels_path, *incompatibility_path, *nerd_file_path;
  bool has_header, classic, partial_observation, force_entire, force_head,
      increasing_demotion, no_inference;
  float threshold, promotion, demotion, testing_ratio;
  unsigned int breadth, experiment_run, max_rules;
  size_t iterations;
  unsigned long s1, s2;
} Arguments;

static char args_doc[] = "DATASET-PATH LABELS-PATH THRESHOLD PROMOTION "
                         "DEMOTION BREADTH EXPERIMENT-ID";

static struct argp_option options[] = {
    {"classic", 'c', 0, 0,
     "Use classic approach. Default: back-ward chaining."},
    {"increasing-demotion", 'd', 0, 0,
     "Enables increasing demotion for back-ward chaining."},
    {"iterations", 'e', "ITERATIONS", 0,
     "Total number of iteration to train Nerd of DATASET-PATH."},
    {"use-entire", 'f', 0, 0,
     "Forces the program to use the entire dataset to training. Default "
     "behaviour will split it into a training and testing."},
    {"force-head", 'h', 0, 0,
     "Forces the created rules to always have a label as their head."},
    {"no-header", 'H', 0, 0,
     "Use if file does not have a header. Default behaviour assume the first "
     "line of DATASET-PATH is where the headers are."},
    {"incompatibility", 'i', "FILE-PATH", 0,
     "Path of a file containing icompatibility rules."},
    {"no-inference", 'I', 0, 0,
     "Forces nerd to not use an inference engine (Prudens-JS)."},
    {"nerd-file", 'n', "NERD-FILEPATH", 0, "The path of an existing .nd file."},
    {"partial-observation", 'p', 0, 0, "The file is partially observed."},
    {"rules", 'r', "MAX-RULES", 0,
     "Total number of rules to learn. Default value: 5."},
    {"state-seed", 's', "STATE-SEED", 0,
     "The starting state seed of the PRNG."},
    {"sequence-seed", 'S', "SEQUENCE-SEED", 0,
     "The sequence seed of the PRNG."},
    {"testing-ratio", 't', "Between [0-1]", 0,
     "The testing dataset ratio. Default value: 0.2."}};

void check_file_existance(char *path, struct argp_state *state) {
  FILE *file = fopen(path, "r");
  if (!file) {
    printf("'%s' is not a valid file path.", path);
    argp_usage(state);
  }
  fclose(file);
}

void check_positive_no_err(char *endptr, char *string,
                           struct argp_state *state) {
  if ((*endptr) || strstr(string, "-")) {
    printf("'%s' should be positive.", string);
    argp_usage(state);
  }
}

error_t parse_opt(int key, char *arg, struct argp_state *state) {
  Arguments *arguments = state->input;
  char *endptr;
  switch (key) {
  case 'c':
    arguments->classic = true;
    break;
  case 'd':
    arguments->increasing_demotion = true;
    break;
  case 'e':
    arguments->iterations = strtoul(arg, &endptr, DECIMAL_BASE);
    check_positive_no_err(endptr, arg, state);
    break;
  case 'f':
    arguments->force_entire = true;
    break;
  case 'h':
    arguments->force_head = true;
    break;
  case 'H':
    arguments->has_header = false;
    break;
  case 'i':
    arguments->incompatibility_path = arg;
    check_file_existance(arg, state);
    break;
  case 'I':
    arguments->no_inference = true;
    break;
  case 'n':
    arguments->nerd_file_path = arg;
    Nerd *nerd = nerd_constructor_from_file(arg, true);
    if (!nerd) {
      printf("'%s' has a bad format.", arg);
      argp_usage(state);
    }
    nerd_destructor(&nerd);
    break;
  case 'p':
    arguments->partial_observation = true;
    break;
  case 'r':
    arguments->max_rules = strtoul(arg, &endptr, DECIMAL_BASE);
    check_positive_no_err(endptr, arg, state);
    break;
  case 's':
    arguments->s1 = strtoul(arg, &endptr, DECIMAL_BASE);
    check_positive_no_err(endptr, arg, state);
    break;
  case 'S':
    arguments->s2 = strtoul(arg, &endptr, DECIMAL_BASE);
    check_positive_no_err(endptr, arg, state);
    break;
  case ARGP_KEY_ARG:
    if (state->arg_num > 7)
      argp_usage(state);

    switch (state->arg_num) {
    case 0:
      arguments->dataset_path = arg;
      check_file_existance(arg, state);
      break;
    case 1:
      arguments->labels_path = arg;
      check_file_existance(arg, state);
      break;
    case 2:
      arguments->threshold = strtof(arg, &endptr);
      check_positive_no_err(endptr, arg, state);
      break;
    case 3:
      arguments->promotion = strtof(arg, &endptr);
      check_positive_no_err(endptr, arg, state);
      break;
    case 4:
      arguments->demotion = strtof(arg, &endptr);
      check_positive_no_err(endptr, arg, state);
      break;
    case 5:
      arguments->breadth = strtoul(arg, &endptr, DECIMAL_BASE);
      check_positive_no_err(endptr, arg, state);
      break;
    case 6:
      arguments->experiment_run = strtoul(arg, &endptr, DECIMAL_BASE);
      check_positive_no_err(endptr, arg, state);
      break;
    }
    break;
  case ARGP_KEY_END:
    if (state->arg_num < 7)
      argp_usage(state);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static Arguments arguments;

static struct argp argp = {options, parse_opt, args_doc, 0};

int main(int argc, char *argv[]) {
  arguments.classic = false;
  arguments.force_entire = false;
  arguments.force_head = false;
  arguments.incompatibility_path = NULL;
  arguments.increasing_demotion = false;
  arguments.has_header = true;
  arguments.iterations = 1;
  arguments.max_rules = 5;
  arguments.nerd_file_path = NULL;
  arguments.no_inference = false;
  arguments.partial_observation = false;
  arguments.testing_ratio = 0.2;
  arguments.s1 = 0;
  arguments.s2 = 0;

  if (argp_parse(&argp, argc, argv, 0, 0, &arguments))
    return EXIT_FAILURE;

  pcg32_random_t generator;

  pcg32_srandom_r(&generator, STATE_SEED, SEQUENCE_SEED);

  char *test_directory = NULL;
  struct timespec current_time;
  size_t time_nanoseconds;
  char *info_file;

  do {
    timespec_get(&current_time, TIME_UTC);
    time_nanoseconds =
        current_time.tv_sec * 1e9 + current_time.tv_nsec + STATE_SEED;

    test_directory = (char *)realloc(
        test_directory, (snprintf(NULL, 0, TEST_DIRECTORY, time_nanoseconds,
                                  arguments.experiment_run) +
                         1) *
                            sizeof(char));

    sprintf(test_directory, TEST_DIRECTORY, time_nanoseconds,
            arguments.experiment_run);
  } while (mkdir(test_directory, 0740) != 0);

  if (arguments.s1 == 0)
    arguments.s1 = time_nanoseconds;

  if (arguments.s2 == 0)
    arguments.s2 =
        arguments.experiment_run + (SEQUENCE_SEED * current_time.tv_nsec);

  pcg32_srandom_r(&generator, arguments.s1, arguments.s2);

  info_file = (char *)malloc(
      (strlen(test_directory) + strlen(INFO_FILE_NAME) + 1) * sizeof(char));
  sprintf(info_file, "%s%s", test_directory, INFO_FILE_NAME);

  umask(S_IROTH | S_IWOTH | S_IWGRP);
  FILE *file = NULL;
  if (!(file = fopen(info_file, "w"))) {
    free(info_file);
    return EXIT_FAILURE;
  }
  free(info_file);

  char *dataset_value = realpath(arguments.dataset_path, NULL);
  fprintf(file, "f=%s\nt=%f\np=%f\nd=%f\nb=%u\ne=%zu\nr=%u\nrun=%d\n",
          dataset_value, arguments.threshold, arguments.promotion,
          arguments.demotion, arguments.breadth, arguments.iterations,
          arguments.max_rules, arguments.experiment_run);

  if (arguments.classic) {
    fprintf(file, "c=true\n");
  } else {
    fprintf(file, "c=false\n");
    if (arguments.increasing_demotion) {
      fprintf(file, "di=true\n");
    } else {
      fprintf(file, "di=false\n");
    }
  }

  if (arguments.partial_observation) {
    fprintf(file, "o=true\n");
  } else {
    fprintf(file, "o=false\n");
  }

  if (arguments.has_header) {
    fprintf(file, "h=true\n");
  } else {
    fprintf(file, "h=false\n");
  }

  if (arguments.force_entire) {
    fprintf(file, "entire=true\n");
  } else {
    fprintf(file, "entire=false\n");
    fprintf(file, "testing_ratio=%f\n", arguments.testing_ratio);
  }

  if (arguments.incompatibility_path) {
    char *abs_incompatibility_path =
        realpath(arguments.incompatibility_path, NULL);
    fprintf(file, "i=%s\n", abs_incompatibility_path);
    free(abs_incompatibility_path);
  }
  fprintf(file, "state_seed=%zu\nseq_seed=%zu\n", arguments.s1, arguments.s2);

  fclose(file);
  global_rng = &generator;

  char *train_path = NULL;
  FILE *dataset = fopen(arguments.dataset_path, "r");

  if (!arguments.force_entire) {
    pcg32_random_t split_rng;
    pcg32_srandom_r(&split_rng, arguments.s1, arguments.s2);
    train_path = (char *)calloc((strlen(TRAIN) + strlen(test_directory) + 1),
                                sizeof(char));
    sprintf(train_path, "%s%s", test_directory, TRAIN);
    if (train_test_split(dataset, arguments.has_header, arguments.testing_ratio,
                         &split_rng, train_path, NULL, NULL, NULL) != 0) {
      free(train_path);
      free(test_directory);
      return EXIT_FAILURE;
    }
  } else {
    train_path = strdup(dataset_value);
  }
  free(dataset_value);
  fclose(dataset);

  char delimiter = ' ';

  if (strstr(arguments.dataset_path, ".csv")) {
    delimiter = ',';
  }

  Sensor *training_dataset = sensor_constructor_from_file(
      train_path, delimiter, true, arguments.has_header);

  if (arguments.breadth == 0)
    arguments.breadth = training_dataset->header_size - 1;

  Nerd *nerd = nerd_constructor(arguments.threshold, arguments.max_rules,
                                arguments.breadth, 1, arguments.promotion,
                                arguments.demotion, !arguments.classic,
                                arguments.increasing_demotion);

  if (arguments.nerd_file_path) {
    Nerd *given_nerd =
        nerd_constructor_from_file(arguments.nerd_file_path, false);
    knowledge_base_destructor(&(nerd->knowledge_base));
    knowledge_base_copy(&(nerd->knowledge_base), given_nerd->knowledge_base);
    nerd_destructor(&given_nerd);
  }

  char experiment_run[5];
  sprintf(experiment_run, "%u", arguments.experiment_run);

  prudensjs_settings_constructor(
      argv[0], test_directory, arguments.incompatibility_path, experiment_run);

  size_t total_instances = sensor_get_total_observations(training_dataset);
  const size_t iterations_str_size =
      snprintf(NULL, 0, "%zu", arguments.iterations);
  const size_t instances_str_size = snprintf(NULL, 0, "%zu", total_instances);
  char *nerd_at_instance_filename = NULL;
  Scene *observation = NULL;
  Scene **incompatibilities = NULL;
  unsigned int i;

  if (arguments.has_header) {
    incompatibilities =
        (Scene **)malloc(training_dataset->header_size * sizeof(Scene *));

    for (i = 0; i < training_dataset->header_size; ++i) {
      incompatibilities[i] = scene_constructor(true);
    }

    unsigned int j, k;
    char *literal, *header;
    Literal *copy;
    for (i = 0; i < total_instances; ++i) {
      sensor_get_next_scene(training_dataset, &observation);

      for (j = 0; j < observation->size; ++j) {
        literal = literal_to_string(observation->literals[j]);
        for (k = 0; k < training_dataset->header_size; ++k) {
          header = (char *)calloc(strlen(training_dataset->header[k]) + 2,
                                  sizeof(char));
          sprintf(header, "%s_", training_dataset->header[k]);
          if (strstr(literal, header)) {
            literal_copy(&copy, observation->literals[j]);
            scene_add_literal(incompatibilities[k], &copy);
            literal_destructor(&copy);
            break;
          }
          safe_free(header);
        }
        safe_free(literal);
      }

      scene_destructor(&observation);
    }
  }

  FILE *labels_file = fopen(arguments.labels_path, "r");

  Context *labels = context_constructor(true);
  Literal *l = NULL;
  unsigned int j = 0;
  int c;
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);

  while ((c = fgetc(labels_file)) != EOF) {
    if ((c == delimiter) || c == '\n') {
      l = literal_constructor_from_string(buffer);
      context_add_literal(labels, &l);
      memset(buffer, 0, strlen(buffer));
      j = 0;
    } else {
      buffer[j++] = c;
    }
  }

  fclose(labels_file);

  size_t iteration, instance,
      total_nerd_time_taken = 0, total_prudens_time_taken = 0,
      current_iteration_nerd_time, current_iteration_prudens_time,
      nerd_time_taken = 0, prudens_time_taken = 0;
  for (iteration = 0; iteration < arguments.iterations; ++iteration) {
    current_iteration_nerd_time = 0;
    current_iteration_prudens_time = 0;
    for (instance = 0; instance < total_instances; ++instance) {
      printf("Iteration %zu of %zu, Instance %zu of %zu\n", iteration + 1,
             arguments.iterations, instance + 1, total_instances);
      sensor_get_next_scene(training_dataset, &observation);

      nerd_train(nerd, arguments.no_inference ? NULL : prudensjs_inference,
                 observation, labels, arguments.force_head, &nerd_time_taken,
                 &prudens_time_taken, training_dataset->header,
                 training_dataset->header_size, incompatibilities);
      total_nerd_time_taken += nerd_time_taken;
      current_iteration_nerd_time += nerd_time_taken;
      total_prudens_time_taken += prudens_time_taken;
      current_iteration_prudens_time += prudens_time_taken;

      scene_destructor(&observation);
      if (test_directory) {
        size_t current_allocated = 0;
        unsigned int z;

        nerd_at_instance_filename = (char *)calloc(
            (snprintf(NULL, 0, "%siteration_%zu-instance_%zu.nd",
                      test_directory, arguments.iterations, total_instances) +
             1),
            sizeof(char));
        sprintf(nerd_at_instance_filename, "%siteration_", test_directory);
        current_allocated = strlen(nerd_at_instance_filename);
        for (z = 0; z < (iterations_str_size -
                         snprintf(NULL, 0, "%zu", iteration + 1));
             ++z) {
          sprintf(nerd_at_instance_filename + (current_allocated++), "0");
        }
        sprintf(nerd_at_instance_filename + current_allocated, "%zu-instance_",
                iteration + 1);
        current_allocated = strlen(nerd_at_instance_filename);
        for (z = 0;
             z < (instances_str_size - snprintf(NULL, 0, "%zu", instance + 1));
             ++z) {
          sprintf(nerd_at_instance_filename + (current_allocated++), "0");
        }
        sprintf(nerd_at_instance_filename + current_allocated, "%zu.nd",
                instance + 1);

        nerd_to_file(nerd, nerd_at_instance_filename);
        safe_free(nerd_at_instance_filename);
      }
    }
    printf("\nTime nerd took for iteration %zu: %zu ms\n", iteration + 1,
           current_iteration_nerd_time);
    printf("Time prudens took for iteration %zu: %zu ms\n\n", iteration + 1,
           current_iteration_prudens_time);
  }

  printf("\nTime spend on nerd: %zu ms\n", total_nerd_time_taken);
  printf("Time spent on prudens: %zu ms\n", total_prudens_time_taken);
  printf("Total time: %zu ms\n\n",
         total_nerd_time_taken + total_prudens_time_taken);

  for (i = 0; i < training_dataset->header_size; ++i) {
    scene_destructor(&(incompatibilities[i]));
  }
  free(incompatibilities);
  sensor_destructor(&training_dataset);
  nerd_destructor(&nerd);
  prudensjs_settings_destructor();

  context_destructor(&labels);
  if (!arguments.force_entire)
    remove(train_path);

  free(train_path);
  free(test_directory);
}
