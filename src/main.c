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

#define TRAIN ".train_set"
#define TEST ".test_set"

int compare(const void *a, const void *b) {
    return (*(int *) a - *(int *) b);
}

int close_dataset(FILE *dataset) {
    if (dataset) {
        fclose(dataset);
    }
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    size_t current_directory_size = strstr(argv[0], "bin/main") - argv[0];
    nerd_current_directory = (char *) malloc((current_directory_size + 1) * sizeof(char));
    memcpy(nerd_current_directory, argv[0], current_directory_size);

    if (argc < 17) {
        printf("Parameters required:\n-f <filepath> The path of the file,\n-i <filepath> The path "
        "of a file containing incompatibility rules in the form of prudens-js,\n-h <bool> Does the "
        "file have a header or not?,\n-t <float> > 0 Threshold value. Must be bigger than 0,\n-p "
        "<float> > 0 Promotion rate. Must be greater than 0,\n-d <float> > 0 Demotion rate. Must be"
        " greater than 0,\n-c <bool> Should it use back-chaining demotion or not? and\n-b <unsigned"
        " int> Maximum number of literals per rule. Must be positive. If more that the maximum "
        "number or 0 are given, it will use the maximum value (max - 1).\nBy adding an additional "
        "number at the end of these parameters, you can change the seed of the random algorithm\n");
        return EXIT_FAILURE;
    }

    bool has_header = false, use_back_chaining = false;
    char opt, delimiter = ' ';
    FILE *dataset = NULL;
    float threshold = INFINITY, promotion = INFINITY, demotion = INFINITY;
    unsigned int i, breadth = 0;
    for (i = 1;
    i < (((unsigned int) argc % 2 == 0) ? (unsigned int) (argc - 1) : (unsigned int) argc); ++i) {
        if (i % 2 == 1) {
            if (sscanf(argv[i], "-%c", &opt) == 1) {
                switch (opt) {
                    case 'f':
                        if (!(dataset = fopen(argv[++i], "r"))) {
                            printf("'-f' value '%s' does not exit.\n", argv[i]);
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
                            return close_dataset(dataset);
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
                            return close_dataset(dataset);
                        }
                        break;
                    case 't':
                        threshold = atof(argv[++i]);
                        if (threshold <= 0) {
                            printf("'-t' has a wrong value '%s'. It must be greater than 0.0\n",
                            argv[i]);
                            return close_dataset(dataset);
                        }
                        break;
                    case 'p':
                        promotion = atof(argv[++i]);
                        if (promotion <= 0) {
                            printf("'-p' has a wrong value '%s'. It must be greater than 0.0\n",
                            argv[i]);
                            return close_dataset(dataset);
                        }
                        break;
                    case 'd':
                        demotion = atof(argv[++i]);
                        if (demotion <= 0) {
                            printf("'-d' has a wrong value '%s'. It must be greater than 0.0\n",
                            argv[i]);
                            return close_dataset(dataset);
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
                            return close_dataset(dataset);
                        }
                        break;
                    case 'b':
                        if (isdigit(argv[++i][0])) {
                            int temp = atoi(argv[i]);
                            if (temp >= 0) {
                                breadth = temp;
                                break;
                            }
                        }
                        printf("'-b' has a wrong value '%s'. It must be an unsigned int\n", argv[i]);
                        return close_dataset(dataset);
                    default:
                        printf("Option '-%c' is not available.\n", opt);
                            return close_dataset(dataset);
                }
            } else {
                printf("Invalid parameters.\n");
                return close_dataset(dataset);
            }
        }
    }

    pcg32_random_t seed;

    pcg32_srandom_r(&seed, time(NULL), 314159U);

    char *directory = NULL;
    if (argc == 18) {
        if (isdigit(argv[17][0])) {
            struct timespec current_time;
            int given_number = atoi(argv[17]);
            size_t time_milliseconds = 0, random_seed = 314159U + given_number;
            char *file_info;

            do {
                timespec_get(&current_time, TIME_UTC);
                time_milliseconds = current_time.tv_sec * 1e9 + current_time.tv_nsec;
                pcg32_srandom_r(&seed, time_milliseconds, random_seed);

                directory = (char *) realloc(directory, (snprintf(NULL, 0, "timestamp=%zu_test=%d_/"
                "filepath=%s", time_milliseconds, given_number, argv[1]) + 1) * sizeof(char));

                sprintf(directory, "timestamp=%zu_test=%d_filepath=%s/", time_milliseconds,
                given_number, argv[1]);
            } while (mkdir(directory, 0740) != 0);

            file_info = (char *) malloc((strlen(directory) + 6) * sizeof(char));
            sprintf(file_info, "%s/info", directory);

            umask(S_IROTH | S_IWOTH | S_IWGRP);
            FILE *file = NULL;
            if (!(file = fopen(file_info, "w"))) {
                free (file_info);
                return close_dataset(dataset);
            }
            free(file_info);

            fprintf(file, "f=%s\nt=%s\np=%s\nd=%s\nc=%s\nb=%s\ntest=%d\nseed1=%zu\nseed2=%zu"
            "\n", argv[2], argv[8], argv[10], argv[12], argv[14], argv[16], given_number,
            time_milliseconds, random_seed);

            fclose(file);
        }
    }
    global_seed = &seed;
    test_directory = directory;
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
    for (i = 0; i < test_size; ++i) {
        current_index = pcg32_random_r(global_seed) % remaining--;
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
    nerd_constructor(train_path, delimiter, true, has_header, threshold, breadth, 1, 1, promotion,
    demotion, use_back_chaining, true);

    nerd_start_learning(nerd);

    nerd_destructor(&nerd);

    remove(train_path);
    remove(test_path);
    free(train_path);
    free(test_path);

    free(nerd_current_directory);
    nerd_current_directory = NULL;
    free(test_directory);
    test_directory = NULL;
    constraints_file = NULL;
    return EXIT_SUCCESS;
}
