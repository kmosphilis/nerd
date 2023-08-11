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
    if (argc != 2) {
        printf("Nerd info filepaths are required.\n");
    }

    FILE *info = NULL, *dataset = NULL;
    info = fopen(argv[1], "rb");
    if (!info) {
        printf("'%s' filepath for info file not found.\n", argv[1]);
        return EXIT_FAILURE;
    }

    size_t buffer_size = BUFFER_SIZE;
    size_t i = 0;
    char *buffer = (char *) calloc(buffer_size, sizeof(char)), *end = NULL;

    bool has_header = false, header_set = false;
    unsigned long *state_seed = NULL, *seq_seed = NULL, temp;
    int c;

    while ((c = fgetc(info)) != EOF) {
        if (c == '\n') {
            char *value = strchr(buffer, '=') + 1;
            if (strstr(buffer, "f=")) {
                if (!(dataset = fopen(value, "r"))) {
                    printf("'f' value '%s' is not valid. Filepath does not exist.\n", value);
                    goto failed;
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
            }

            memset(buffer, 0, strlen(buffer));
            i = 0;
        } else {
            buffer[i++] = c;

            if (i == buffer_size) {
                buffer_size <<= 2;
                buffer = (char *) realloc(buffer, buffer_size);
                memset(buffer + (buffer_size >> 2), 0, buffer_size >> 2);
            }
        }
    }
    safe_free(buffer);
    fclose(info);
    info = NULL;

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
    *testing_path = (char *) calloc(strlen(test_dir) + strlen(TESTING_FILE) + 1, sizeof(char));

    sprintf(training_path, "%s%s", test_dir, TRAINING_FILE);
    sprintf(testing_path, "%s%s", test_dir, TESTING_FILE);
    free(test_dir);

    umask(S_IROTH | S_IWOTH | S_IWGRP);
    train_test_split(dataset, has_header, 0.2, &rng, training_path, testing_path, NULL, NULL);
    free(training_path);
    free(testing_path);
    fclose(dataset);

    return EXIT_SUCCESS;
}
