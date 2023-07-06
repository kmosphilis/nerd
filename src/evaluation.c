#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "nerd_helper.h"
#include "nerd_utils.h"
#include "metrics.h"

#define BUFFER_SIZE 256
#define DECIMAL_BASE 10

#define TRAIN ".train_set"
#define TEST ".test_set"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Nerd info filepath, nerd file (.nd) and labels file required!\n");
        return EXIT_FAILURE;
    }

    FILE *info_file = fopen(argv[1], "r");
    if (!info_file) {
        printf("Info file does not exist. Please provide a valid path.\n");
        return EXIT_FAILURE;
    }

    int c;
    size_t value_buffer_size = BUFFER_SIZE;
    char *value_buffer = (char *) malloc(value_buffer_size), *end = NULL;
    memset(value_buffer, 0, value_buffer_size);
    unsigned int i = 0;

    FILE *dataset = NULL;
    size_t state_seed, seq_seed;
    float threshold, promotion, demotion;
    unsigned int breadth = 0;
    int given_number;
    char delimiter= ' ';
    bool use_back_chaining = true, partial_observation = true, has_header = false;
    char *constraints_file = NULL;

    while ((c = fgetc(info_file)) != EOF) {
        if (c == '\n') {
            char *equals_location = strstr(value_buffer, "=");
            if (equals_location) {
                int equals_index = equals_location - value_buffer;
                const char *true_value = equals_location + 1;
                char *option = (char *) malloc(equals_index + 1);
                memcpy(option, value_buffer, equals_index);
                switch (option[0]) {
                    case 'f':
                        if (!(dataset = fopen(true_value, "r"))) {
                            goto failed;
                        }
                        if (strstr(true_value, ".csv")) {
                            delimiter = ',';
                        }
                        break;
                    case 't':
                        threshold = strtof(true_value, &end);
                        if (*end) {
                            goto option_failed;
                        }
                        break;
                    case 'p':
                        promotion = strtof(true_value, &end);
                        if (*end) {
                            goto option_failed;
                        }
                        break;
                    case 'd':
                        demotion = strtof(true_value, &end);
                        if (*end) {
                            goto option_failed;
                        }
                        break;
                    case 'b':
                        breadth = strtoul(true_value, &end, DECIMAL_BASE);
                        if (*end) {
                            goto option_failed;
                        }
                        break;
                    case 'r':
                        given_number = strtol(true_value, &end, DECIMAL_BASE);
                        if (*end) {
                            goto option_failed;
                        }
                        break;
                    case 'c':
                        if (strcmp(true_value, "true") == 0) {
                            use_back_chaining = false;
                        }
                        break;
                    case 'o':
                        if (strcmp(true_value, "false") == 0) {
                            partial_observation = false;
                        }
                        break;
                    case 'h':
                        if (strcmp(true_value, "true") == 0) {
                            has_header = true;
                        }
                        break;
                    case 'i':
                        constraints_file = strdup(true_value);
                        break;
                    case 's':
                        if (strcmp(option, "state_seed")) {
                            state_seed = strtoul(true_value, &end, DECIMAL_BASE);
                            if (*end) {
                                goto option_failed;
                            }
                        } else if (strcmp(option, "seq_seed")) {
                            seq_seed = strtoul(true_value, &end, DECIMAL_BASE);
                            if (*end) {
                                goto option_failed;
                            }
                        } else {
                            goto option_failed;
                        }
                        break;
                    default:
option_failed:
                        free(option);
                        goto failed;

                }
                safe_free(option);
            } else {
failed:
                if (dataset) {
                    fclose(dataset);
                }
                free(value_buffer);
                fclose(info_file);
                i = 0;
                return EXIT_FAILURE;
            }
            memset(value_buffer, 0, strlen(value_buffer));
            i = 0;
        } else {
            if (i == value_buffer_size) {
                value_buffer_size *= 2;
                value_buffer = (char *) realloc(value_buffer, value_buffer_size);
                memset(value_buffer + (value_buffer_size / 2), 0, value_buffer_size / 2);
            }
            value_buffer[i++] = c;
        }
    }
    fclose(info_file);
    free(value_buffer);

    Nerd *nerd = NULL;
    unsigned int epoch_number;
    if (strstr(argv[2], ".nd")) {
        nerd = nerd_constructor_from_file(argv[2], 0, use_back_chaining, false);
        if (!(nerd && (sscanf(strstr(argv[2], "epoch-"), "epoch-%u.nd", &epoch_number) == 1))) {
            nerd_destructor(&nerd);
            fclose(dataset);
            printf("Nerd file has a bad format %u.\n", epoch_number);
            return EXIT_FAILURE;
        }
    } else {
        printf("Please provide a .nd file\n");
        return EXIT_FAILURE;
    }

    FILE *labels_file;
    Context *labels;
    Literal *l;
    if ((labels_file = fopen(argv[3], "rb"))) {
        labels = context_constructor(true);
        int c;
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        char labels_delimiter = ' ';
        if (strstr(argv[3], ".csv")) {
            labels_delimiter = ',';
        }

        while ((c = fgetc(labels_file)) != EOF) {
            if ((c == labels_delimiter) || c == '\n') {
                l = literal_constructor_from_string(buffer);
                context_add_literal(labels, &l);
                memset(buffer, 0, strlen(buffer));
                i = 0;
            } else {
                buffer[i++] = c;
            }
        }

        fclose(labels_file);
    } else {
        printf("Labels file cannot be accessed!\n");
        return EXIT_FAILURE;
    }

    pcg32_random_t seed;
    pcg32_srandom_r(&seed, state_seed, seq_seed);

    char *last_slash = strrchr(argv[2], '/') + 1;

    char *test_directory = (char *) malloc((last_slash - argv[2] + 1) * sizeof(char));
    memcpy(test_directory, argv[2], last_slash - argv[2]);

    PrudensSettings_ptr settings;
    prudensjs_settings_constructor(&settings, argv[0], test_directory, constraints_file,
    strstr(argv[2], "epoch"));

    char *train_path = (char *) calloc((snprintf(NULL, 0, "%s%s%u", test_directory, TRAIN,
    epoch_number) + 1), sizeof(char)),
    *test_path = (char *) calloc((snprintf(NULL, 0, "%s%s%u", test_directory, TEST, epoch_number)
    + 1), sizeof(char));
    sprintf(train_path, "%s%s%u", test_directory, TRAIN, epoch_number);
    sprintf(test_path, "%s%s%u", test_directory, TEST, epoch_number);


    train_test_split(dataset, has_header, 0.2, &seed, train_path, test_path, NULL, NULL);
    fclose(dataset);

    float accuracy, abstain;

    char *train_results_name = (char *) calloc((strlen("/train_results") + strlen(test_directory)
    + 1), sizeof(char)), *test_results_name = (char *) calloc((strlen("/test_results")
    + strlen(test_directory) + 1), sizeof(char));
    sprintf(train_results_name, "%s/train_results", test_directory);
    sprintf(test_results_name, "%s/test_results", test_directory);
    FILE *train_results = fopen(train_results_name, "ab+"),
    *test_results = fopen(test_results_name, "ab+");
    free(train_results_name);
    free(test_results_name);

    evaluate_labels(nerd, settings, train_path, labels, delimiter, has_header, &accuracy, &abstain);
    fprintf(train_results, "%u %f %f\n", epoch_number, accuracy, abstain);
    fclose(train_results);

    evaluate_labels(nerd, settings, test_path, labels, delimiter, has_header, &accuracy, &abstain);
    fprintf(test_results, "%u %f %f\n", epoch_number, accuracy, abstain);
    fclose(test_results);

    prudensjs_settings_destructor(&settings);
    remove(train_path);
    free(train_path);
    remove(test_path);
    free(test_path);
    free(test_directory);
    context_destructor(&labels);
    nerd_destructor(&nerd);
    return EXIT_SUCCESS;
}
