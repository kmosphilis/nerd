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
#define STATE_SEED 314159U
#define SEQUENCE_SEED 271828U

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
    if (argc < 13) {
        printf("Required parameters:\n-f <filepath> The path of the file,\n-h <bool> Does the file "
        "have a header or not?,\n-t <float> > 0 Threshold value. It must be bigger than 0,\n-p "
        "<float> > 0 Promotion rate. Must be greater than 0,\n-d <float> > 0 Demotion rate. Must be"
        " greater than 0, and\n-b <unsigned int> Maximum number of literals per rule. It must be\n "
        "positive. If more that the maximum number or 0 are given, it will use\n the maximum value "
        "(max - 1).\n\nOptional parameters:\n-i <filepath> The path of a file containing "
        "incompatibility rules in the\n form of prudens-js.\n-e <unsigned long> Number of epochs "
        "that Nerd should train for. It must\n be greater than 0. Default value is set to 1.\n-c "
        "<bool> Should it use the classic approach or not (use back-chaining\n demotion)? Default "
        "value is set to 'false'\n-o <bool> Should the it be trained using partial observations? "
        "Default\n value is set to 'true'.-l <filepath> This option enables the rule learning to "
        "focus on these\n labels found in the given file. \n-s1 <unsigned long>  Seed 1 (state "
        "seed) for the PRNG. It must be bigger\n than 0.\n\nBy adding an additional number at the "
        "end of these parameters, you mark\n the number of this run and it will save its given "
        "parameters.\n");
        return EXIT_FAILURE;
    }

    Context *labels = NULL;
    bool has_header = false, use_back_chaining = true, partial_observation = true,
    should_evaluate = false, s1_set = false, s2_set = false;
    char opt, delimiter = ' ';
    FILE *dataset = NULL;
    float threshold = INFINITY, promotion = INFINITY, demotion = INFINITY;
    unsigned int i, breadth = 0;
    char *current_arg, *arg_end, *dataset_value = NULL, *constraints_file = NULL;
    size_t epochs = 1, s1 = 0, s2 = 0;
    for (i = 1;
    i < (((unsigned int) argc % 2 == 0) ? (unsigned int) (argc - 1) : (unsigned int) argc); ++i) {
        if (i % 2 == 1) {
            if ((strlen(argv[i]) > 2) && strstr(argv[i], "-")) {
                char *large_option = argv[i] + 1;
                current_arg = argv[++i];
                if (strcmp(large_option, "s1") == 0) {
                    s1 = strtoul(current_arg, &arg_end, DECIMAL_BASE);
                    if ((*arg_end) || strstr(current_arg, "-")) {
                        printf("'-s1' value '%s' is not valid. It must be an unsigned long greater "
                        "than 0 (> 0).\n", large_option);
                        return close_dataset_and_exit(dataset);
                    }
                    s1_set = true;
                } else if (strcmp(large_option, "s2") == 0) {
                    s2 = strtoul(current_arg, &arg_end, DECIMAL_BASE);
                    if ((*arg_end) || strstr(current_arg, "-")) {
                        printf("'-s2' value '%s' is not valid. It must be an unsigned long greater "
                        "than 0 (> 0).\n", large_option);
                        return close_dataset_and_exit(dataset);
                    }
                    s2_set = true;
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
                        break;
                    case 'e':
                        current_arg = argv[++i];
                        epochs = strtoul(current_arg, &arg_end, DECIMAL_BASE);
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
    pcg32_random_t generator;

    pcg32_srandom_r(&generator, STATE_SEED, SEQUENCE_SEED);

    char *test_directory = NULL;
    if ((argc >= 18) && (argc % 2 == 0)) {
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
            fprintf(file, "f=%s\nt=%f\np=%f\nd=%f\nb=%u\nrun=%d\n",
            dataset_value, threshold, promotion, demotion, breadth, given_number);
            free(dataset_value);

            if (!use_back_chaining) {
                fprintf(file, "c=true\n");
            } else {
                fprintf(file, "c=false\n");
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

    char *train_path = (char *) malloc((strlen(TRAIN) + strlen(test_directory) + 1) * sizeof(char));

    sprintf(train_path, "%s%s", test_directory, TRAIN);

    if (train_test_split(dataset, has_header, 0.2, global_rng, train_path, NULL, NULL, NULL) != 0) {
        context_destructor(&labels);
        free(train_path);
        free(test_directory);
        return EXIT_FAILURE;
    }
    fclose(dataset);

    Nerd *nerd =
    nerd_constructor(train_path, delimiter, true, has_header, threshold, breadth, 1, epochs,
    promotion, demotion, use_back_chaining, partial_observation);

    PrudensSettings_ptr settings = NULL;
    prudensjs_settings_constructor(&settings, argv[0], test_directory, constraints_file,
    current_arg);

    nerd_start_learning(nerd, settings, test_directory, NULL, labels);

    nerd_destructor(&nerd);
    prudensjs_settings_destructor(&settings);
    context_destructor(&labels);

    remove(train_path);
    free(train_path);

    free(test_directory);
    return EXIT_SUCCESS;
}
