#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <pcg_variants.h>

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
    if (argc != 13) {
        printf("Parameters required -f <filepath> The path of the file,\n-h <bool> Does the file "
        "have a header or not?,\n-t <float> > 0 Threshold value. Must be bigger than 0,\n-p <float>"
        " > 0 Promotion rate. Must be greater than 0,\n-d <float> > 0 Demotion rate. Must be "
        "greater than 0 and,\n-b <bool> Should it use back-chaining demotion or not?");
        return EXIT_FAILURE;
    }

    bool has_header = false, use_back_chaining = false;
    char opt, delimiter = ' ';
    FILE *dataset = NULL;
    float threshold = INFINITY, promotion = INFINITY, demotion = INFINITY;
    unsigned int i;
    for (i = 1; i < (unsigned int) argc; ++i) {
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
                    case 'b':
                        if (strcmp(argv[++i], "true") == 0) {
                            use_back_chaining = true;
                        } else if (strcmp(argv[i], "false") == 0) {
                            use_back_chaining = false;
                        } else {
                            printf("'-h' has a wrong value '%s'. It must be a boolean value, 'true'"
                            " or 'false'\n", argv[i]);
                            return close_dataset(dataset);
                        }
                        break;
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

    size_t dataset_size = 0;

    int c;
    for (c = fgetc(dataset); c != EOF; c = fgetc(dataset)) {
        if (c == '\n') {
            ++dataset_size;
        }
    }
    dataset_size -= 2;
    fseek(dataset, 0, SEEK_SET);

    FILE *train = fopen(TRAIN, "wb"), *test = fopen(TEST, "wb");

    do {
        c = fgetc(dataset);
        fputc(c, train);
        fputc(c, test);
    } while (c != '\n');

    size_t test_size = dataset_size * 0.2;
    fpos_t start_of_file;

    fgetpos(dataset, &start_of_file);

    unsigned int *possible_indices = (unsigned int *) malloc(dataset_size * sizeof(int)),
    *test_indices = (unsigned int *) malloc(test_size * sizeof(int));

    for (i = 0; i < dataset_size; ++i) {
        possible_indices[i] = i;
    }

    int current_index;
    size_t remaining = dataset_size;
    for (i = 0; i < test_size; ++i) {
        current_index = pcg32_random_r(&seed) % remaining--;
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

    printf("%c, %d\n", delimiter, has_header);

    Nerd *nerd =
    nerd_constructor(TRAIN, delimiter, true, has_header, threshold, 3, 1, 1, promotion, demotion,
    use_back_chaining, true);

    nerd_start_learning(nerd);

    nerd_destructor(&nerd);

    remove(TEST);
    remove(TRAIN);

    return EXIT_SUCCESS;
}
