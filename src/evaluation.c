#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nerd_helper.h"
#include "nerd_utils.h"
#include "metrics.h"

#define BUFFER_SIZE 256
#define DECIMAL_BASE 10

#define RESULT_DIR "results/"
#define TRAIN ".train_set"
#define TEST ".test_set"

int main(int argc, char *argv[]) {
    if ((argc != 4) && (argc != 6)) {
        printf("Nerd info filepath, nerd file (.nd) and labels file required, and optionally a"
        "testing datset and if it has a header (boolean).\n");
        return EXIT_FAILURE;
    }

    FILE *info_file = fopen(argv[1], "r");
    if (!info_file) {
        printf("Info file does not exist. Please provide a valid path.\n");
        return EXIT_FAILURE;
    }
    char *testing_dataset_path = NULL, testing_delimiter = ' ';
    bool testing_has_header = false;
    if (argc == 6) {
        testing_dataset_path = strdup(argv[4]);
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

        if (strcmp(argv[5], "true") == 0) {
            testing_has_header = true;
        }
    }

    int c;
    size_t value_buffer_size = BUFFER_SIZE;
    char *value_buffer = (char *) calloc(value_buffer_size, sizeof(char)), *end = NULL;
    size_t i = 0;

    FILE *dataset = NULL;
    size_t state_seed, seq_seed;
    char training_delimiter= ' ';
    bool use_back_chaining = true, training_has_header = false, entire = false,
    partial_observation = false;
    char *constraints_file = NULL, *dataset_value = NULL;
    float testing_ratio = 0.2;

    while ((c = fgetc(info_file)) != EOF) {
        if (c == '\n') {
            char *equals_location = strstr(value_buffer, "=");
            if (equals_location) {
                int equals_index = equals_location - value_buffer;
                char *true_value = equals_location + 1;
                char *option = (char *) calloc(equals_index + 1, sizeof(char));
                memcpy(option, value_buffer, equals_index);
                switch (option[0]) {
                    case 'f':
                        if (!(dataset = fopen(true_value, "r"))) {
                            printf("Filepath '%s' does not exist.\n", true_value);
                            goto failed;
                        }
                        dataset_value = strdup(true_value);
                        if (strstr(true_value, ".csv")) {
                            training_delimiter = ',';
                        }
                        break;
                    case 'h':
                        if (strcmp(true_value, "true") == 0) {
                            training_has_header = true;
                        }
                        break;
                    case 'i':
                        constraints_file = strdup(true_value);
                        break;
                    case 's':
                        if (strcmp(option, "state_seed") == 0) {
                            state_seed = strtoul(true_value, &end, DECIMAL_BASE);
                            if (*end) {
                                printf("%s is not a valid state_seed value. It required an unsigned"
                                " long.\n", true_value);
                                goto option_failed1;
                            }
                        } else if (strcmp(option, "seq_seed") == 0) {
                            seq_seed = strtoul(true_value, &end, DECIMAL_BASE);
                            if (*end) {
                                printf("%s is not a valid seq_seed value. It required an unsigned"
                                " long.\n", true_value);
                                goto option_failed1;
                            }
                        } else {
                            goto option_failed2;
                        }
                        break;
                    case 't':
                        if (strcmp(option, "testing_ratio") == 0) {
                            testing_ratio = strtof(true_value, &end);
                            if (*end || (testing_ratio < 0) || (testing_ratio > 1)) {
                                printf("'-ratio' value '%s' is not valid. It must be a real number "
                                "between [0,1]\n", true_value);
                                goto option_failed1;
                            }
                        }
                        break;
                    case 'p':
                    case 'd':
                    case 'b':
                    case 'e':
                        if (strcmp(option, "entire") == 0) {
                            if (strcmp(true_value, "true") == 0) {
                                entire = true;
                            }
                        }
                        break;
                    case 'r':
                    case 'c':
                    case 'o':
                        if (strcmp(true_value, "true") == 0) {
                            partial_observation = true;
                        }
                        break;
                    default:
option_failed2:
                        printf("Option '%s' is not valid.\n", option);
option_failed1:
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
                free(testing_dataset_path);
                free(dataset_value);
                i = 0;
                return EXIT_FAILURE;
            }
            memset(value_buffer, 0, strlen(value_buffer));
            i = 0;
        } else {
            if ((i + 1) == value_buffer_size) {
                value_buffer_size <<= 1;
                value_buffer = (char *) realloc(value_buffer, value_buffer_size);
                memset(value_buffer + (value_buffer_size >> 1), 0, value_buffer_size >> 1);
            }
            value_buffer[i++] = c;
        }
    }
    fclose(info_file);
    free(value_buffer);

    Nerd *nerd = NULL;
    unsigned int iteration_number, instance_number;
    if (strstr(argv[2], ".nd")) {
        nerd = nerd_constructor_from_file(argv[2], use_back_chaining);
        if (!(nerd && (sscanf(strstr(argv[2], "iteration_"), "iteration_%u-instance_%u.nd",
        &iteration_number, &instance_number) == 2))) {
            nerd_destructor(&nerd);
            fclose(dataset);
            printf("Nerd file has a bad format.\n");
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

    char *last_slash = strrchr(argv[2], '/') + 1;

    char *test_directory = (char *) calloc(((last_slash - argv[2]) + 1), sizeof(char));
    memcpy(test_directory, argv[2], last_slash - argv[2]);

    char *result_directory = (char *) calloc((strlen(test_directory) + strlen(RESULT_DIR) + 1),
    sizeof(char));
    sprintf(result_directory, "%s%s", test_directory, RESULT_DIR);

    if (mkdir(result_directory, 0740) != 0) {
        if (errno != EEXIST) {
            free(test_directory);
            free(result_directory);
            return EXIT_FAILURE;
        }
    }
    free(test_directory);

    pcg32_random_t seed;
    pcg32_srandom_r(&seed, state_seed, seq_seed);

    prudensjs_settings_constructor(argv[0], result_directory, constraints_file,
    strstr(argv[2], "iteration"));

    char *train_path = NULL, *test_path = NULL;
    if (entire) {
        train_path = dataset_value;
        test_path = testing_dataset_path;
    } else {
        train_path = (char *) calloc(snprintf(NULL, 0, "%s%s%u", result_directory, TRAIN,
        iteration_number) + 1, sizeof(char));
        sprintf(train_path, "%s%s%u", result_directory, TRAIN, iteration_number);

        if (testing_dataset_path) {
            test_path = testing_dataset_path;

            train_test_split(dataset, training_has_header, testing_ratio, &seed, train_path, NULL, NULL,
            NULL);
        } else {
            test_path = (char *) calloc(snprintf(NULL, 0, "%s%s%u", result_directory, TEST,
            iteration_number) + 1, sizeof(char));
            sprintf(test_path, "%s%s%u", result_directory, TEST, iteration_number);

            train_test_split(dataset, training_has_header, testing_ratio, &seed, train_path, test_path, NULL,
            NULL);
            testing_delimiter = training_delimiter;
            testing_has_header = training_has_header;
        }

        free(dataset_value);
    }
    fclose(dataset);

    char *instance_directory =
    (char *) calloc((strlen(result_directory) + strlen(last_slash) + 1 + 1), sizeof(char));
    sprintf(instance_directory, "%s%s/", result_directory, last_slash);

    if (mkdir(instance_directory, 0740) != 0) {
        if (errno != EEXIST) {
            free(result_directory);
            free(instance_directory);
            return EXIT_FAILURE;
        }
    }
    free(result_directory);

    char *train_results_name = (char *) calloc(strlen(instance_directory) + strlen("train.txt") + 1,
    sizeof(char));
    sprintf(train_results_name, "%strain.txt", instance_directory);

    char *test_results_name = (char *) calloc(strlen(instance_directory) + strlen("test.txt") + 1,
    sizeof(char));
    sprintf(test_results_name, "%stest.txt", instance_directory);

    char *train_results_rules_name = (char *) calloc(strlen(instance_directory) +
    strlen("train_rules.txt") + 1, sizeof(char));
    sprintf(train_results_rules_name, "%strain_rules.txt", instance_directory);

    char *test_results_rules_name = (char *) calloc(strlen(instance_directory) +
    strlen("test_rules.txt") + 1, sizeof(char));
    sprintf(test_results_rules_name, "%stest_rules.txt", instance_directory);

    free(instance_directory);

    umask(S_IROTH | S_IWOTH | S_IWGRP);
    FILE *train_results = fopen(train_results_name, "wb"),
    *test_results = fopen(test_results_name, "wb"),
    *train_rules = fopen(train_results_rules_name, "wb"),
    *test_rules = fopen(test_results_rules_name, "wb");
    free(train_results_name);
    free(test_results_name);
    free(train_results_rules_name);
    free(test_results_rules_name);

    size_t total_observations;
    Scene **result = NULL;
    char *str;
    unsigned int j, k;
    Sensor *training_dataset = sensor_constructor_from_file(train_path, training_delimiter, false,
    training_has_header),
    *testing_dataset = sensor_constructor_from_file(test_path, testing_delimiter, false,
    testing_has_header), *datasets[2] = {training_dataset, testing_dataset};
    char *paths[2] = {train_path, test_path};
    FILE *files[2] = {train_results, test_results};
    FILE *rule_files[2] = {train_rules, test_rules};

    char *rules = NULL;
    for (k = 0; k < 2; ++k) {
        if (evaluate_labels(nerd, prudensjs_inference_batch, datasets[k], labels, NULL, NULL, &total_observations,
        NULL, &result, &rules, partial_observation) == 0) {
            for (i = 0; i < total_observations; ++i) {
                for (j = 0; j < result[i]->size; ++j) {
                    if (j != 0) {
                        fprintf(files[k], " ");
                    }
                    str = literal_to_string(result[i]->literals[j]);
                    fprintf(files[k], "%s", str);
                    safe_free(str);
                }
                fprintf(files[k], "\n");
                scene_destructor(&(result[i]));
            }
        }
        fprintf(rule_files[k], "%s\n", rules);

        safe_free(rules);
        safe_free(result);
        fclose(files[k]);
        fclose(rule_files[k]);
        sensor_destructor(&(datasets[k]));
        if ((paths[k] != dataset_value) && (paths[k] != testing_dataset_path)) {
            remove(paths[k]);
        }
        free(paths[k]);
    }

    free(constraints_file);
    prudensjs_settings_destructor();
    context_destructor(&labels);
    nerd_destructor(&nerd);
    return EXIT_SUCCESS;
}
