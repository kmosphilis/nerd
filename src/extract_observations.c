#include <errno.h>
#include <pcg_variants.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "nerd_utils.h"
#include "scene.h"
#include "sensor.h"

#define BUFFER_SIZE 256
#define DECIMAL_BASE 10
#define RESULTS "results/"
#define TRAINING_FILE "training-dataset.txt"
#define TESTING_FILE "testing-dataset.txt"

int main(int argc, char *argv[]) {
  if ((argc != 2) && (argc != 4)) {
    printf("Nerd info filepath is required, and optionally a testing dataset "
           "and if it has"
           "a header (boolean).\n");
  }

  FILE *info_file = NULL, *dataset = NULL;
  info_file = fopen(argv[1], "rb");
  if (!info_file) {
    printf("'%s' filepath for info file not found.\n", argv[1]);
    return EXIT_FAILURE;
  }

  char *testing_dataset_path = NULL, testing_delimiter = ' ';
  bool testing_has_header = false;
  if (argc == 4) {
    testing_dataset_path = strdup(argv[2]);
    FILE *test = fopen(testing_dataset_path, "r");
    if (!test) {
      printf("Testing dataset %s is not valid.\n", testing_dataset_path);
      fclose(info_file);
      return EXIT_FAILURE;
    }
    fclose(test);

    if (strstr(testing_dataset_path, ".csv")) {
      testing_delimiter = ',';
    }

    if (strcmp(argv[3], "true") == 0) {
      testing_has_header = true;
    }
  }

  size_t buffer_size = BUFFER_SIZE;
  size_t i = 0;
  char *buffer = (char *)calloc(buffer_size, sizeof(char)), *end = NULL,
       *dataset_value = NULL, training_delimiter = ' ';

  bool training_has_header = false, header_set = false, entire = false;
  unsigned long *state_seed = NULL, *seq_seed = NULL, temp;
  int c;
  float testing_ratio = 0.2;

  while ((c = fgetc(info_file)) != EOF) {
    if (c == '\n') {
      char *value = strchr(buffer, '=') + 1;
      if (strstr(buffer, "f=")) {
        dataset_value = strdup(value);
        if (!(dataset = fopen(value, "r"))) {
          printf("'f' value '%s' is not valid. Filepath does not exist.\n",
                 value);
          goto failed;
        }

        if (strstr(buffer, ".csv")) {
          training_delimiter = ',';
        }
      } else if (strstr(buffer, "state_seed=")) {
        temp = strtoul(value, &end, DECIMAL_BASE);
        if (*end) {
          printf("'state_seed' value '%s' is not valid. It should be an "
                 "unsigned long.\n",
                 value);
          goto failed;
        }
        state_seed = (unsigned long *)malloc(sizeof(long));
        *state_seed = temp;
      } else if (strstr(buffer, "seq_seed=")) {
        temp = strtoul(value, &end, DECIMAL_BASE);
        if (*end) {
          printf("'seq_seed' value '%s' is not valid. It should be an unsigned "
                 "long.\n",
                 value);
          goto failed;
        }
        seq_seed = (unsigned long *)malloc(sizeof(long));
        *seq_seed = temp;
      } else if (strstr(buffer, "h=")) {
        if (strcmp(value, "true") == 0) {
          training_has_header = true;
        } else if (strcmp(buffer, "false")) {
          training_has_header = false;
        } else {
          printf("'h' value '%s' is not valid. It should be either 'true' or "
                 "'false'\n",
                 value);
          goto failed;
        }
        header_set = true;
      } else if (strstr(buffer, "entire=")) {
        if (strcmp(value, "true") == 0) {
          entire = true;
        } else if (strcmp(buffer, "false")) {
          entire = false;
        } else {
          printf("'entire' value '%s' is not valid. It should be either 'true' "
                 "or 'false'"
                 "\n",
                 value);
          goto failed;
        }
      } else if (strstr(buffer, "testing_ratio=")) {
        testing_ratio = strtof(value, &end);
        if (*end || (testing_ratio < 0) || (testing_ratio > 1)) {
          printf("'-ratio' value '%s' is not valid. It must be a real number "
                 "between "
                 "[0,1]\n",
                 value);
          goto failed;
        }
      }

      memset(buffer, 0, strlen(buffer));
      i = 0;
    } else {
      buffer[i++] = c;

      if ((i + 1) == buffer_size) {
        buffer_size <<= 1;
        buffer = (char *)realloc(buffer, buffer_size);
        memset(buffer + (buffer_size >> 1), 0, buffer_size >> 1);
      }
    }
  }
  safe_free(buffer);
  fclose(info_file);
  info_file = NULL;

  if (!header_set || !state_seed || !seq_seed || !dataset) {
    if (!state_seed) {
      printf("state_seed option was not found.\n");
    } else if (!seq_seed) {
      printf("seq_seed option was not found.\n");
    } else if (!dataset) {
      printf("f option was not found.\n");
    } else {
      printf("h option was not found.\n");
    }
  failed:
    fclose(dataset);
    free(state_seed);
    free(seq_seed);
    free(buffer);
    free(testing_dataset_path);
    free(dataset_value);
    return EXIT_FAILURE;
  }

  pcg32_random_t rng;

  pcg32_srandom_r(&rng, *state_seed, *seq_seed);
  free(state_seed);
  free(seq_seed);

  char *last_slash = strrchr(argv[1], '/') + 1;
  char *test_dir = (char *)calloc((last_slash - argv[1]) + strlen(RESULTS) + 1,
                                  sizeof(char));
  memcpy(test_dir, argv[1], last_slash - argv[1]);
  sprintf(test_dir + (last_slash - argv[1]), "%s", RESULTS);

  if (mkdir(test_dir, 0740) != 0) {
    if (errno != EEXIST) {
      free(test_dir);
      fclose(dataset);
      return EXIT_FAILURE;
    }
  }

  char *training_file_path = (char *)calloc(
           strlen(test_dir) + strlen(TRAINING_FILE) + 1, sizeof(char)),
       *testing_file_path = (char *)calloc(
           strlen(test_dir) + strlen(TESTING_FILE) + 1, sizeof(char)),
       *training_set_path = NULL, *testing_set_path = NULL;

  sprintf(training_file_path, "%s%s", test_dir, TRAINING_FILE);
  sprintf(testing_file_path, "%s%s", test_dir, TESTING_FILE);

  umask(S_IROTH | S_IWOTH | S_IWGRP);
  if (entire) {
    training_set_path = dataset_value;
    testing_set_path = testing_dataset_path;
  } else {
    training_set_path = (char *)calloc(
        strlen(test_dir) + strlen(TRAINING_FILE) + 2, sizeof(char));
    sprintf(training_set_path, "%s.%s", test_dir, TRAINING_FILE);

    if (testing_dataset_path) {
      testing_set_path = testing_dataset_path;

      train_test_split(dataset, training_has_header, testing_ratio, &rng,
                       training_set_path, NULL, NULL, NULL);
    } else {
      testing_set_path = (char *)calloc(
          strlen(test_dir) + strlen(TESTING_FILE) + 2, sizeof(char));
      sprintf(testing_set_path, "%s.%s", test_dir, TESTING_FILE);

      train_test_split(dataset, training_has_header, testing_ratio, &rng,
                       training_set_path, testing_set_path, NULL, NULL);

      testing_delimiter = training_delimiter;
      testing_has_header = training_has_header;
    }

    free(dataset_value);
  }
  free(test_dir);
  fclose(dataset);

  Sensor *sensor = NULL;
  Scene *observation = NULL;

  char *file_paths[] = {training_file_path, testing_file_path};
  char *temp_paths[] = {training_set_path, testing_set_path};
  char delimiters[] = {training_delimiter, testing_delimiter};
  bool headers[] = {training_has_header, testing_has_header};
  char *str = NULL;
  FILE *file;

  size_t j;
  for (i = 0; i < 2; ++i) {
    file = fopen(file_paths[i], "wb");
    sensor = sensor_constructor_from_file(temp_paths[i], delimiters[i], false,
                                          headers[i]);
    free(file_paths[i]);

    sensor_get_next_scene(sensor, &observation);
    for (; observation; sensor_get_next_scene(sensor, &observation)) {
      for (j = 0; j < observation->size; ++j) {
        if (j != 0) {
          fprintf(file, " ");
        }
        str = literal_to_string(observation->literals[j]);
        fprintf(file, "%s", str);
        safe_free(str);
      }
      fprintf(file, "\n");

      scene_destructor(&observation);
    }

    sensor_destructor(&sensor);
    fclose(file);
    if ((temp_paths[i] != dataset_value) &&
        (temp_paths[i] != testing_dataset_path)) {
      remove(temp_paths[i]);
    }
    free(temp_paths[i]);
  }

  return EXIT_SUCCESS;
}
