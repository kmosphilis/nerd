#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pcg_variants.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "scene.h"
#include "sensor.h"
#include "nerd_utils.h"

#define BUFFER_SIZE 256
#define DECIMAL_BASE 10
#define RESULTS "results/"
#define TRAINING_FILE "training-dataset.txt"
#define TESTING_FILE "testing-dataset.txt"

int main(int argc, char *argv[]) {
    if ((argc < 2) && (argc > 3)) {
        printf("Nerd info filepath is required, and optionally a testing dataset.\n");
    }

    FILE *info_file = NULL, *dataset = NULL;
    info_file = fopen(argv[1], "rb");
    if (!info_file) {
        printf("'%s' filepath for info file not found.\n", argv[1]);
        return EXIT_FAILURE;
    }

    char *testing_dataset = NULL;
    if (argc == 3) {
        testing_dataset = strdup(argv[2]);
        FILE *test = fopen(testing_dataset, "r");
        if (!test) {
            printf("Testing dataset %s is not valid.\n", testing_dataset);
            fclose(info_file);
            return EXIT_FAILURE;
        }
        fclose(test);

    }

    size_t buffer_size = BUFFER_SIZE;
    size_t i = 0;
    char *buffer = (char *) calloc(buffer_size, sizeof(char)), *end = NULL, *dataset_value = NULL,
    delimiter = ' ';

    bool has_header = false, header_set = false, entire = false;
    unsigned long *state_seed = NULL, *seq_seed = NULL, temp;
    int c;

    while ((c = fgetc(info_file)) != EOF) {
        if (c == '\n') {
            char *value = strchr(buffer, '=') + 1;
            if (strstr(buffer, "f=")) {
                dataset_value = strdup(value);
                if (!(dataset = fopen(value, "r"))) {
                    printf("'f' value '%s' is not valid. Filepath does not exist.\n", value);
                    goto failed;
                }

                if (strstr(buffer, ".csv")) {
                    delimiter = ',';
                }
            } else if (strstr(buffer, "state_seed=")) {
                temp = strtoul(value, &end, DECIMAL_BASE);
                if (*end) {
                    printf("'state_seed' value '%s' is not valid. It should be an unsigned long.\n",
                    value);
                    goto failed;
                }
                state_seed = (unsigned long *) malloc(sizeof(long));
                *state_seed = temp;
            } else if (strstr(buffer, "seq_seed=")) {
                temp = strtoul(value, &end, DECIMAL_BASE);
                if (*end) {
                    printf("'seq_seed' value '%s' is not valid. It should be an unsigned long.\n",
                    value);
                    goto failed;
                }
                seq_seed = (unsigned long *) malloc(sizeof(long));
                *seq_seed = temp;
            } else if (strstr(buffer, "h=")) {
                if (strcmp(value, "true") == 0) {
                    has_header = true;
                } else if (strcmp(buffer, "false")) {
                    has_header = false;
                } else {
                    printf("'h' value '%s' is not valid. It should be either 'true' or 'false'\n",
                    value);
                    goto failed;
                }
                header_set = true;
            } else if(strstr(buffer, "entire=")) {
                if (strcmp(value, "true") == 0) {
                    entire = true;
                } else if (strcmp(buffer, "false")) {
                    entire = false;
                } else {
                    printf("'entire' value '%s' is not valid. It should be either 'true' or 'false'"
                    "\n", value);
                    goto failed;
                }
            }

            memset(buffer, 0, strlen(buffer));
            i = 0;
        } else {
            buffer[i++] = c;

            if ((i + 1) == buffer_size) {
                buffer_size <<= 1;
                buffer = (char *) realloc(buffer, buffer_size);
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
        free(testing_dataset);
        free(dataset_value);
        return EXIT_FAILURE;
    }

    pcg32_random_t rng;

    pcg32_srandom_r(&rng, *state_seed, *seq_seed);
    free(state_seed);
    free(seq_seed);

    char *last_slash = strrchr(argv[1], '/') + 1;
    char *test_dir = (char *) calloc((last_slash - argv[1]) + strlen(RESULTS) + 1, sizeof(char));
    memcpy(test_dir, argv[1], last_slash - argv[1]);
    sprintf(test_dir + (last_slash - argv[1]), "%s", RESULTS);

    if (mkdir(test_dir, 0740) != 0) {
        if (errno != EEXIST) {
            free(test_dir);
            fclose(dataset);
            return EXIT_FAILURE;
        }
    }

    char *training_path =
    (char *) calloc(strlen(test_dir) + strlen(TRAINING_FILE) + 1, sizeof(char)),
    *testing_path = (char *) calloc(strlen(test_dir) + strlen(TESTING_FILE) + 1, sizeof(char)),
    *training_temp_path = NULL, *testing_temp_path = NULL;

    sprintf(training_path, "%s%s", test_dir, TRAINING_FILE);
    sprintf(testing_path, "%s%s", test_dir, TESTING_FILE);

    umask(S_IROTH | S_IWOTH | S_IWGRP);
    if (entire) {
        training_temp_path = dataset_value;
        testing_temp_path = testing_dataset;
    } else {
        training_temp_path =
        (char *) calloc(strlen(test_dir) + strlen(TRAINING_FILE) + 2, sizeof(char));
        testing_temp_path =
        (char *) calloc(strlen(test_dir) + strlen(TESTING_FILE) + 2, sizeof(char));
        sprintf(training_temp_path, "%s.%s", test_dir, TRAINING_FILE);
        sprintf(testing_temp_path, "%s.%s", test_dir, TESTING_FILE);

        train_test_split(dataset, has_header, 0.2, &rng, training_temp_path, testing_temp_path,
        NULL, NULL);
        free(dataset_value);
    }
    free(test_dir);
    fclose(dataset);

    Sensor *sensor = NULL;
    Scene *observation = NULL;

    char *file_paths[] = {training_path, testing_path};
    char *temp_paths[] = {training_temp_path, testing_temp_path};
    char *str = NULL;
    FILE *file;

    size_t j;
    for (i = 0; i < 2; ++i) {
        file = fopen(file_paths[i], "wb");
        sensor = sensor_constructor_from_file(temp_paths[i], delimiter, false, has_header);
        free(file_paths[i]);

        sensor_get_next_scene(sensor, &observation, false, NULL);
        for (;observation; sensor_get_next_scene(sensor, &observation, false, NULL)) {
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
        if (!entire) {
            remove(temp_paths[i]);
        }
        free(temp_paths[i]);
    }

    return EXIT_SUCCESS;
}
