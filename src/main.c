#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <pcg_variants.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nerd.h"
#include "nerd_helper.h"
#include "metrics.h"

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

int main(int argc, char *argv[]) {
    int exit_code = EXIT_SUCCESS;
    if (argc < 13) {
        printf("Required parameters:\n-f <filepath> The path of the file,\n-h <bool> Does the file "
        "have a header or not?,\n-t <float> > 0 Threshold value. It must be bigger than 0,\n-p "
        "<float> > 0 Promotion rate. Must be greater than 0,\n-d <float> > 0 Demotion rate. Must be"
        " greater than 0, and\n-b <unsigned int> Maximum number of literals per rule. It must be\n "
        "positive. If more that the maximum number or 0 are given, it will use\n the maximum value "
        "(max - 1).\n\nOptional parameters:\n-i <filepath> The path of a file containing "
        "incompatibility rules in the\n form of prudens-js.\n-e <unsigned long> Number of "
        "iterations that Nerd should train for. It must\n be greater than 0. Default value is set "
        "to 1.\n-r <unsigned int> Number of maximum rules to be learnt\n at each instance/"
        "observation. It must be greater than 0, Default value\n is set to 5.\n-c <bool> Should it "
        "use the classic approach or not (use backward chaining\n demotion)? Default value is set "
        "to 'false'\n-di <bool> Should the demotion rate be increasing or decreasing for\n deeper "
        "causing rules? 'true' for increasing, 'false' for decreasing.\n Default value is set to "
        "'false'.\n-o <bool> Should the it be trained using partial observations? Default\n value "
        "is set to 'true'.\n-l <filepath> This option enables the rule learning to focus on these\n"
        " labels found in the given file.\n-s1 <unsigned long>  Seed 1 (state seed) for the PRNG. "
        "\n-s2 <unsigned long>  Seed 2 (sequence seed) for the PRNG.\n It must be bigger than 0.\n"
        "-kb <filepath> The path to an .nd file which contains a KB to\n be re-used.\n-entire "
        "<bool> Should it use the given dataset for training only?\n Default value is set to "
        "'false'.\n-ratio <float> The testing dataset ratio. Must be between [0,1]. Default\n value"
        " is set to 0.2\n\nBy adding an additional number at the end of these parameters, you mark"
        "\n the number of this run and it will save its given parameters.\n");
        return EXIT_FAILURE;
    }

    Context *labels = NULL;
    bool h_set = false, b_set = false;
    bool has_header = false, use_back_chaining = true, partial_observation = true,
    should_evaluate = false, s1_set = false, s2_set = false, increasing_demotion = false,
    entire = false;
    char opt, delimiter = ' ';
    FILE *dataset = NULL;
    float threshold = INFINITY, promotion = INFINITY, demotion = INFINITY, testing_ratio = 0.2;
    unsigned int i, breadth = 0, max_rules_per_instance = 5;
    char *current_arg, *arg_end, *dataset_value = NULL, *constraints_file = NULL, *kb = NULL;
    size_t iterations = 1, s1 = 0, s2 = 0;
    for (i = 1;
    i < (((unsigned int) argc % 2 == 0) ? (unsigned int) (argc - 1) : (unsigned int) argc); ++i) {
        if (i % 2 == 1) {
            if ((strlen(argv[i]) > 2) && strstr(argv[i], "-")) {
                char *large_option = argv[i] + 1;
                current_arg = argv[++i];
                if (strcmp(large_option, "di") == 0) {
                    if (strcmp(current_arg, "true") == 0) {
                        increasing_demotion = false;
                    } else if (strcmp(current_arg, "false") == 0) {
                        increasing_demotion = true;
                    } else {
                        printf("'-di' has a wrong value '%s'. It must be a boolean value, 'true' or"
                        " 'false'\n", current_arg);
                        return close_dataset_and_exit(dataset);
                    }
                } else if (strcmp(large_option, "s1") == 0) {
                    s1 = strtoul(current_arg, &arg_end, DECIMAL_BASE);
                    if ((*arg_end) || strstr(current_arg, "-")) {
                        printf("'-s1' value '%s' is not valid. It must be an unsigned long greater "
                        "than 0 (> 0).\n", current_arg);
                        return close_dataset_and_exit(dataset);
                    }
                    s1_set = true;
                } else if (strcmp(large_option, "s2") == 0) {
                    s2 = strtoul(current_arg, &arg_end, DECIMAL_BASE);
                    if ((*arg_end) || strstr(current_arg, "-")) {
                        printf("'-s2' value '%s' is not valid. It must be an unsigned long greater "
                        "than 0 (> 0).\n", current_arg);
                        return close_dataset_and_exit(dataset);
                    }
                    s2_set = true;
                } else if (strcmp(large_option, "kb") == 0) {
                    kb = current_arg;
                } else if (strcmp(large_option, "entire") == 0) {
                    if (strcmp(current_arg, "true") == 0) {
                            entire = true;
                        } else if (strcmp(current_arg, "false") == 0) {
                            entire = false;
                        } else {
                            printf("'-entire' has a wrong value '%s'. It must be a boolean value, "
                            "'true' or 'false'\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                } else if (strcmp(large_option, "ratio") == 0) {
                    testing_ratio = strtof(current_arg, &arg_end);
                    if ((*arg_end) || (testing_ratio < 0) || (testing_ratio > 1)) {
                        printf("'-ratio' value '%s' is not valid. It must be a real number between "
                        "[0,1]\n", current_arg);
                        return close_dataset_and_exit(dataset);
                    }
                } else {
                    printf("Option '-%s' is not available.\n", large_option);
                    return close_dataset_and_exit(dataset);
                }
            } else if (sscanf(argv[i], "-%c", &opt) == 1) {
                switch (opt) {
                    case 'f':
                        current_arg = argv[++i];
                        if (!(dataset = fopen(current_arg, "r"))) {
                            printf("'-f' value '%s' does not exist.\n", current_arg);
                            return EXIT_FAILURE;
                        }
                        if (strstr(current_arg, ".csv")) {
                            delimiter = ',';
                        } else {
                            delimiter = ' ';
                        }
                        dataset_value = current_arg;
                        break;
                    case 'i':
                        current_arg = argv[++i];
                        FILE *incompatibility_rules = fopen(current_arg, "r");
                        if (!incompatibility_rules) {
                            printf("'-i' value '%s' does not exist.\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        fclose(incompatibility_rules);
                        constraints_file = current_arg;
                        break;
                    case 'h':
                        current_arg = argv[++i];
                        if (strcmp(current_arg, "true") == 0) {
                            has_header = true;
                        } else if (strcmp(current_arg, "false") == 0) {
                            has_header = false;
                        } else {
                            printf("'-h' has a wrong value '%s'. It must be a boolean value, 'true'"
                            " or 'false'\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        h_set = true;
                        break;
                    case 't':
                        current_arg = argv[++i];
                        threshold = strtof(current_arg, &arg_end);
                        if ((*arg_end) || (threshold <= 0)) {
                            printf("'-t' has a wrong value '%s'. It must be a float greater than "
                            "0.0\n", current_arg);

                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'p':
                        current_arg = argv[++i];
                        promotion = strtof(current_arg, &arg_end);
                        if ((*arg_end) || (promotion <= 0)) {
                            printf("'-p' has a wrong value '%s'. It must be a float greater than "
                            "0.0\n", current_arg);

                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'd':
                        current_arg = argv[++i];
                        demotion = strtof(current_arg, &arg_end);
                        if ((*arg_end) || (demotion <= 0)) {
                            printf("'-d' has a wrong value '%s'. It must be a float greater than "
                            "0.0\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'c':
                        current_arg = argv[++i];
                        if (strcmp(current_arg, "true") == 0) {
                            use_back_chaining = false;
                        } else if (strcmp(current_arg, "false") == 0) {
                            use_back_chaining = true;
                        } else {
                            printf("'-c' has a wrong value '%s'. It must be a boolean value, 'true'"
                            " or 'false'\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'b':
                        current_arg = argv[++i];
                        breadth = strtoul(current_arg, &arg_end, DECIMAL_BASE);
                        if ((*arg_end) || strstr(current_arg, "-")) {
                            printf("'-b' has a wrong value '%s'. It must be an unsigned int\n",
                            current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        b_set = true;
                        break;
                    case 'e':
                        current_arg = argv[++i];
                        iterations = strtoul(current_arg, &arg_end, DECIMAL_BASE);
                        if ((*arg_end) || strstr(current_arg, "-")) {
                            printf("'-e' has a wrong value '%s'. It must be an unsigned long "
                            "greater than 0\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'o':
                        current_arg = argv[++i];
                        if (strcmp(current_arg, "true") == 0) {
                            partial_observation = true;
                        } else if (strcmp(current_arg, "false") == 0) {
                            partial_observation = false;
                        } else {
                            printf("'-o' has a wrong value '%s'. It must be a boolean value, 'true'"
                            " or 'false'\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'l':
                        current_arg = argv[++i];
                        FILE *labels_file = fopen(current_arg, "r");
                        if (!labels_file) {
                            printf("'-l' value '%s' does not exist.\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }

                        char labels_delimiter = ' ';
                        if (strstr(current_arg, ".csv")) {
                            labels_delimiter = ',';
                        }

                        labels = context_constructor(true);
                        Literal *l = NULL;
                        unsigned int j = 0;
                        int c;
                        char buffer[BUFFER_SIZE];
                        memset(buffer, 0, BUFFER_SIZE);

                        while ((c = fgetc(labels_file)) != EOF) {
                            if ((c == labels_delimiter) || c == '\n') {
                                l = literal_constructor_from_string(buffer);
                                context_add_literal(labels, &l);
                                memset(buffer, 0, strlen(buffer));
                                j = 0;
                            } else {
                                buffer[j++] = c;
                            }
                        }

                        fclose(labels_file);
                        break;
                    case 'r':
                        current_arg = argv[++i];
                        max_rules_per_instance = strtoul(current_arg, &arg_end, DECIMAL_BASE);
                        if ((*arg_end) || strstr(current_arg, "-")) {
                            printf("'-r' has a wrong value '%s'. It must be an unsigned int greater"
                            " than 0\n", current_arg);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    // case 'v':
                    //     current_arg = argv[++i];
                    //     if (strcmp(current_arg, "true") == 0) {
                    //         should_evaluate = true;
                    //     } else if (strcmp(current_arg, "false") == 0) {
                    //         should_evaluate = false;
                    //     } else {
                    //         printf("'-v' has a wrong value '%s'. It must be a boolean value, 'true'"
                    //         " or 'false'\n", current_arg);
                    //         return close_dataset_and_exit(dataset);
                    //     }
                    //     break;
                    default:
                        printf("Option '-%c' is not available.\n", opt);
                        return close_dataset_and_exit(dataset);
                }
            } else {
                printf("Invalid parameters.\n");
                return close_dataset_and_exit(dataset);
            }
        }
    }

    if (!dataset) {
        printf("'-f' is required.\n");
        exit(1);
    } else if (!h_set) {
        printf("'-h' is required.\n");
        return close_dataset_and_exit(dataset);
    } else if (threshold == INFINITY) {
        printf("'-t' is required.\n");
        return close_dataset_and_exit(dataset);
    } else if (promotion == INFINITY) {
        printf("'-p' is required.\n");
        return close_dataset_and_exit(dataset);
    } else if (demotion == INFINITY) {
        printf("'-d' is required.\n");
        return close_dataset_and_exit(dataset);
    } else if (!b_set) {
        printf("'-b' is required.\n");
        return close_dataset_and_exit(dataset);
    }

    Nerd *given_nerd = NULL;
    if (kb) {
        given_nerd = nerd_constructor_from_file(kb, iterations, use_back_chaining, false);
        if (!given_nerd) {
            printf("%s has a bad format.\n", kb);
            return close_dataset_and_exit(dataset);
        }
    }

    pcg32_random_t generator;

    pcg32_srandom_r(&generator, STATE_SEED, SEQUENCE_SEED);

    char *test_directory = NULL;
    if ((argc >= 14) && (argc % 2 == 0)) {
        if (isdigit(argv[argc - 1][0])) {
            struct timespec current_time;
            current_arg = argv[argc - 1];
            int given_number = atoi(current_arg);
            size_t time_nanoseconds;
            char *info_file;

            do {
                timespec_get(&current_time, TIME_UTC);
                time_nanoseconds = current_time.tv_sec * 1e9 + current_time.tv_nsec + STATE_SEED;

                test_directory = (char *) realloc(test_directory, (snprintf(NULL, 0, TEST_DIRECTORY,
                time_nanoseconds, given_number) + 1) * sizeof(char));

                sprintf(test_directory, TEST_DIRECTORY, time_nanoseconds, given_number);
            } while (mkdir(test_directory, 0740) != 0);

            if (!s1_set) {
                s1 = time_nanoseconds;
            }

            if (!s2_set) {
                s2 = given_number + (SEQUENCE_SEED * current_time.tv_nsec);
            }
            pcg32_srandom_r(&generator, s1, s2);

            info_file = (char *) malloc((strlen(test_directory) + strlen(INFO_FILE_NAME) + 1)
            * sizeof(char));
            sprintf(info_file, "%s%s", test_directory, INFO_FILE_NAME);

            umask(S_IROTH | S_IWOTH | S_IWGRP);
            FILE *file = NULL;
            if (!(file = fopen(info_file, "w"))) {
                free (info_file);
                return close_dataset_and_exit(dataset);
            }
            free(info_file);

            dataset_value = realpath(dataset_value, NULL);
            fprintf(file, "f=%s\nt=%f\np=%f\nd=%f\nb=%u\ne=%zu\nr=%u\nrun=%d\n",
            dataset_value, threshold, promotion, demotion, breadth, iterations,
            max_rules_per_instance, given_number);

            if (!use_back_chaining) {
                fprintf(file, "c=true\n");
            } else {
                fprintf(file, "c=false\n");
                if (increasing_demotion) {
                    fprintf(file, "di=true\n");
                } else {
                    fprintf(file, "di=false\n");
                }
            }

            if (partial_observation) {
                fprintf(file, "o=true\n");
            } else {
                fprintf(file, "o=false\n");
            }

            if (has_header) {
                fprintf(file, "h=true\n");
            } else {
                fprintf(file, "h=false\n");
            }

            if (entire) {
                fprintf(file, "entire=true\n");
            } else {
                fprintf(file, "entire=false\n");
                fprintf(file, "testing_ratio=%f\n", testing_ratio);
            }

            if (constraints_file) {
                char *abs_constraints_file = realpath(constraints_file, NULL);
                fprintf(file, "i=%s\n", abs_constraints_file);
                free(abs_constraints_file);
            }
            fprintf(file, "state_seed=%zu\nseq_seed=%zu\n", s1, s2);

            fclose(file);
        }
    }
    global_rng = &generator;

    char *train_path = NULL;


    if (!entire) {
        train_path = (char *) calloc((strlen(TRAIN) + strlen(test_directory) + 1), sizeof(char));
        sprintf(train_path, "%s%s", test_directory, TRAIN);
        if (train_test_split(dataset, has_header, testing_ratio, global_rng, train_path, NULL, NULL, NULL) != 0) {
            context_destructor(&labels);
            free(train_path);
            free(test_directory);
            return EXIT_FAILURE;
        }
    } else {
        train_path = strdup(dataset_value);
    }
    free(dataset_value);
    fclose(dataset);

    if (labels) {
        Sensor *sensor = sensor_constructor_from_file(train_path, delimiter, false, has_header);
        size_t total_observations = sensor_get_total_observations(sensor);
        Scene *observation, *result;

        for (i = 0; i < total_observations; ++i) {
            sensor_get_next_scene(sensor, &observation, false, NULL);

            scene_intersect(observation, labels, &result);

            if (result->size == 0) {
                char *str = scene_to_string(observation), *labels_str = scene_to_string(labels);
                printf("labels files does not contain a label for observation\nLabels: %s\nObservation:"
                " %s\n", labels_str, str);

                free(str);
                free(labels_str);
                scene_destructor(&result);
                scene_destructor(&observation);
                exit_code = EXIT_FAILURE;
                break;
            }

            scene_destructor(&result);
            scene_destructor(&observation);
        }
        sensor_destructor(&sensor);

        if (exit_code) {
            goto failed;
        }
    }

    if (constraints_file) {
        FILE *constraints = fopen(constraints_file, "r");
        Literal *l1, *l2;
        char *first_literal = (char *) calloc(BUFFER_SIZE, sizeof(char)),
        *second_literal = (char *) calloc(BUFFER_SIZE, sizeof(char));

        i = 0;
        while (fscanf(constraints, "%*s :: %s # %s\n", first_literal, second_literal) == 2) {
            if (strrchr(second_literal, ';')) {
                second_literal[strlen(second_literal) - 1] = '\0';
            } else {
                printf("A simicolon ; is missing at the end of the rule at line %u", i);
                exit_code = EXIT_FAILURE;
                break;
            }

            ++i;
            l1 = literal_constructor_from_string(first_literal);
            l2 = literal_constructor_from_string(second_literal);

            int should_fail = -1;

            if (scene_literal_index(labels, l1) < 0) {
                should_fail = 0;
            } else if (scene_literal_index(labels, l2) < 0) {
                should_fail = 1;
            }
            if (should_fail > -1) {
                Literal *chosen_literal = (should_fail == 0) ? l1 : l2;
                char *str = literal_to_string(chosen_literal);
                printf("Literal: %s in line %u does not exist in the labels. Check again.\n",
                str, i);

                free(str);
                literal_destructor(&l1);
                literal_destructor(&l2);
                exit_code = EXIT_FAILURE;
                break;
            }

            literal_destructor(&l1);
            literal_destructor(&l2);
            memset(first_literal, 0, strlen(first_literal));
            memset(second_literal, 0, strlen(second_literal));
        }
        free(first_literal);
        free(second_literal);
        fclose(constraints);

        if (exit_code) {
            goto failed;
        }
    }

    // Nerd *nerd =
    // if (!nerd) {
    Nerd *nerd = nerd_constructor(train_path, delimiter, true, has_header, threshold,
    max_rules_per_instance, breadth, 1, iterations, promotion, demotion, use_back_chaining,
    increasing_demotion, partial_observation);

    if (given_nerd) {
        knowledge_base_destructor(&(nerd->knowledge_base));
        knowledge_base_copy(&(nerd->knowledge_base), given_nerd->knowledge_base);
        nerd_destructor(&given_nerd);
    }

    nerd_constructor_from_file("", iterations, false, false);


    PrudensSettings_ptr settings = NULL;
    prudensjs_settings_constructor(&settings, argv[0], test_directory, constraints_file,
    current_arg);

    nerd_start_learning(nerd, settings, test_directory, NULL, labels);

    nerd_destructor(&nerd);
    prudensjs_settings_destructor(&settings);
failed:
    context_destructor(&labels);
    if (!entire) {
        remove(train_path);
    }
    free(train_path);

    free(test_directory);
    return exit_code;
}
