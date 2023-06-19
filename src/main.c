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

#define TRAIN ".train_set"
#define TEST ".test_set"
#define INFO_FILE_NAME "info"

#define STATE_SEED 314159U
#define SEQUENCE_SEED 271828U

int compare(const void *a, const void *b) {
    return (*(int *) a - *(int *) b);
}

int close_dataset_and_exit(FILE *dataset) {
    if (dataset) {
        fclose(dataset);
    }
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc < 17) {
        printf("Parameters required:\n-f <filepath> The path of the file,\n-i <filepath> The path "
        "of a file containing incompatibility rules in the form of prudens-js,\n-h <bool> Does the "
        "file have a header or not?,\n-t <float> > 0 Threshold value. Must be bigger than 0,\n-p "
        "<float> > 0 Promotion rate. Must be greater than 0,\n-d <float> > 0 Demotion rate. Must be"
        " greater than 0,\n-c <bool> Should it use back-chaining demotion or not? and\n-b <unsigned"
        " int> Maximum number of literals per rule. Must be positive. If more that the maximum "
        "number or 0 are given, it will use the maximum value (max - 1).\n(Optional) -e <unsigned "
        "long> Number of epochs that Nerd should train for. Must be greater than 0. By default the "
        "epochs are set to 1.\n(Optional) -o <bool> Should the file be trained using partial "
        "observations? By default the value is set to 'true'.\n(Optional) -l <filepath> This option"
        "enables evaluating the learned KnowledgeBase with a file containing the possible labels "
        "that should be predicted.\n(Optional) By adding an additional number at the end of these "
        "parameters, you can change the seed of the random algorithm\n");
        return EXIT_FAILURE;
    }

    Context *labels = NULL;
    bool has_header = false, use_back_chaining = false, partial_observation = true;
    char opt, delimiter = ' ';
    FILE *dataset = NULL;
    float threshold = INFINITY, promotion = INFINITY, demotion = INFINITY;
    unsigned int i, breadth = 0;
    char *constraints_file = NULL;
    size_t epochs = 1;
    for (i = 1;
    i < (((unsigned int) argc % 2 == 0) ? (unsigned int) (argc - 1) : (unsigned int) argc); ++i) {
        if (i % 2 == 1) {
            if (sscanf(argv[i], "-%c", &opt) == 1) {
                switch (opt) {
                    case 'f':
                        if (!(dataset = fopen(argv[++i], "r"))) {
                            printf("'-f' value '%s' does not exist.\n", argv[i]);
                            return EXIT_FAILURE;
                        }
                        if (strstr(argv[i], ".csv")) {
                            delimiter = ',';
                        } else {
                            delimiter = ' ';
                        }
                        break;
                    case 'i':
                        FILE *incompatibility_rules = fopen(argv[++i], "r");
                        if (!incompatibility_rules) {
                            printf("'-i' value '%s' does not exist.\n", argv[i]);
                            return close_dataset_and_exit(dataset);
                        }
                        fclose(incompatibility_rules);
                        constraints_file = argv[i];
                        break;
                    case 'h':
                        if (strcmp(argv[++i], "true") == 0) {
                            has_header = true;
                        } else if (strcmp(argv[i], "false") == 0) {
                            has_header = false;
                        } else {
                            printf("'-h' has a wrong value '%s'. It must be a boolean value, 'true'"
                            " or 'false'\n", argv[i]);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 't':
                        threshold = atof(argv[++i]);
                        if (threshold <= 0) {
                            printf("'-t' has a wrong value '%s'. It must be a float greater than "
                            "0.0\n", argv[i]);

                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'p':
                        promotion = atof(argv[++i]);
                        if (promotion <= 0) {
                            printf("'-p' has a wrong value '%s'. It must be a float greater than "
                            "0.0\n", argv[i]);

                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'd':
                        demotion = atof(argv[++i]);
                        if (demotion <= 0) {
                            printf("'-d' has a wrong value '%s'. It must be a float greater than "
                            "0.0\n", argv[i]);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'c':
                        if (strcmp(argv[++i], "true") == 0) {
                            use_back_chaining = true;
                        } else if (strcmp(argv[i], "false") == 0) {
                            use_back_chaining = false;
                        } else {
                            printf("'-c' has a wrong value '%s'. It must be a boolean value, 'true'"
                            " or 'false'\n", argv[i]);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'b':
                        if ((isdigit(argv[++i][0])) || (isdigit(argv[i][1]))) {
                            breadth = atoi(argv[i]);
                            break;
                        }
                        printf("'-b' has a wrong value '%s'. It must be an unsigned int\n",
                        argv[i]);
                        return close_dataset_and_exit(dataset);
                    case 'e':
                        if ((isdigit(argv[++i][0])) || (isdigit(argv[i][1]))) {
                            epochs = (size_t) atol(argv[i]);
                            if (epochs > 0) {
                                break;
                            }
                        }
                        printf("'-e' has a wrong value '%s'. It must be an unsigned long greater "
                        "than 0\n", argv[i]);
                        return close_dataset_and_exit(dataset);
                    case 'o':
                        if (strcmp(argv[++i], "true") == 0) {
                            partial_observation = true;
                        } else if (strcmp(argv[i], "false") == 0) {
                            use_back_chaining = false;
                        } else {
                            printf("'-o' has a wrong value '%s'. It must be a boolean value, 'true'"
                            " or 'false'\n", argv[i]);
                            return close_dataset_and_exit(dataset);
                        }
                        break;
                    case 'l':
                        FILE *labels_file = fopen(argv[++i], "r");
                        if (!labels_file) {
                            printf("'-l' value '%s' does not exist.\n", argv[i]);
                            return close_dataset_and_exit(dataset);
                        }

                        char labels_delimiter = ' ';
                        if (strstr(argv[i], ".csv")) {
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

    pcg32_random_t seed;

    pcg32_srandom_r(&seed, STATE_SEED, time(NULL));

    char *test_directory = NULL;
    if ((argc >= 18) && (argc % 2 == 0)) {
        if (isdigit(argv[argc - 1][0])) {
            struct timespec current_time;
            int given_number = atoi(argv[argc - 1]);
            size_t time_milliseconds, random_seed;
            char *file_info;

            do {
                timespec_get(&current_time, TIME_UTC);
                random_seed = given_number + (SEQUENCE_SEED * current_time.tv_nsec);
                time_milliseconds = current_time.tv_sec * 1e9 + current_time.tv_nsec + STATE_SEED;
                pcg32_srandom_r(&seed, time_milliseconds, random_seed);

                test_directory = (char *) realloc(test_directory, (snprintf(NULL, 0,
                "timestamp=%zu_test=%d_/filepath=%s", time_milliseconds, given_number, argv[1]) + 1)
                * sizeof(char));

                sprintf(test_directory, "timestamp=%zu_test=%d_filepath=%s/", time_milliseconds,
                given_number, argv[1]);
            } while (mkdir(test_directory, 0740) != 0);

            file_info = (char *) malloc((strlen(test_directory) + strlen(INFO_FILE_NAME) + 1)
            * sizeof(char));
            sprintf(file_info, "%s%s", test_directory, INFO_FILE_NAME);

            umask(S_IROTH | S_IWOTH | S_IWGRP);
            FILE *file = NULL;
            if (!(file = fopen(file_info, "w"))) {
                free (file_info);
                return close_dataset_and_exit(dataset);
            }
            free(file_info);

            fprintf(file, "f=%s\nt=%s\np=%s\nd=%s\nc=%s\nb=%s\ntest=%d\nstate_seed=%zu\n"
            "seq_seed=%zu\ntrain_test_split_state_seed=%d\ntrain_test_split_seq_seed=%d\n",
            argv[2], argv[8], argv[10], argv[12], argv[14], argv[16], given_number,
            time_milliseconds, random_seed, STATE_SEED, SEQUENCE_SEED);

            fclose(file);
        }
    }
    global_seed = &seed;

    size_t dataset_size = 0;
    int c;
    for (c = fgetc(dataset); c != EOF; c = fgetc(dataset)) {
        if (c == '\n') {
            ++dataset_size;
        }
    }

    fseek(dataset, 0, SEEK_SET);

    char *train_path = (char *) malloc((strlen(TRAIN) + strlen(test_directory) + 1) * sizeof(char)),
    *test_path = (char *) malloc((strlen(TEST) + strlen(test_directory) + 1) * sizeof(char));

    sprintf(train_path, "%s%s", test_directory, TRAIN);
    sprintf(test_path, "%s%s", test_directory, TEST);

    FILE *train = fopen(train_path, "wb"), *test = fopen(test_path, "wb");

    if (has_header) {
        dataset_size -= 1;
        do {
            c = fgetc(dataset);
            fputc(c, train);
            fputc(c, test);
        } while (c != '\n');
    }

    size_t test_size = dataset_size * 0.2;
    unsigned int *possible_indices = (unsigned int *) malloc(dataset_size * sizeof(int)),
    *test_indices = (unsigned int *) malloc(test_size * sizeof(int));

    for (i = 0; i < dataset_size; ++i) {
        possible_indices[i] = i;
    }

    int current_index;
    size_t remaining = dataset_size;
    pcg32_random_t train_test_split_seed;
    pcg32_srandom_r(&train_test_split_seed, STATE_SEED, SEQUENCE_SEED);
    for (i = 0; i < test_size; ++i) {
        current_index = pcg32_random_r(&train_test_split_seed) % remaining--;
        test_indices[i] = possible_indices[current_index];
        possible_indices[current_index] = possible_indices[remaining];
    }

    free(possible_indices);

    qsort(test_indices, test_size, sizeof(int), compare);

    FILE *file_to_write;
    unsigned int test_indices_index = 0;
    for (i = 0; i < dataset_size; ++i) {
        if (test_indices[test_indices_index] == i) {
            ++test_indices_index;
            file_to_write = test;
        } else {
            file_to_write = train;
        }

        do {
            c = fgetc(dataset);
            fputc(c, file_to_write);
        } while (c != '\n');
    }
    free(test_indices);
    fclose(train);
    fclose(test);
    fclose(dataset);

    Nerd *nerd =
    nerd_constructor(train_path, delimiter, true, has_header, threshold, breadth, 1, epochs,
    promotion, demotion, use_back_chaining, partial_observation);

    PrudensSettings_ptr settings = NULL;
    prudensjs_settings_constructor(&settings, argv[0], test_directory, constraints_file);

    nerd_start_learning(nerd, settings, test_directory, test_path, labels);

    nerd_destructor(&nerd);
    prudensjs_settings_destructor(&settings);
    context_destructor(&labels);

    remove(train_path);
    remove(test_path);
    free(train_path);
    free(test_path);

    free(test_directory);
    return EXIT_SUCCESS;
}
