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

int main(int argc, char *argv[]) {
    if (argc != 9) {
        printf("Parameters required -f <filepath>, -t <float> > 0, -p <float> > 0 and -d <float> "
        "> 0.\n");
        return EXIT_FAILURE;
    }

    char opt;
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
                        break;
                    case 't':
                        threshold = atof(argv[++i]);
                        if (threshold <= 0) {
                            printf("'-t' has a wrong value %s. It must be more than 0.0\n", argv[i]);
                            return EXIT_FAILURE;
                        }
                        break;
                    case 'p':
                        promotion = atof(argv[++i]);
                        if (promotion <= 0) {
                            printf("'-p' has a wrong value %s. It must be more than 0.0\n", argv[i]);
                            return EXIT_FAILURE;
                        }
                        break;
                    case 'd':
                        demotion = atof(argv[++i]);
                        if (demotion <= 0) {
                            printf("'-d' has a wrong value %s. It must be more than 0.0\n", argv[i]);
                            return EXIT_FAILURE;
                        }
                        break;
                    default:
                        printf("Option '-%c' is not available.\n", opt);
                        return EXIT_FAILURE;
                }
            } else {
                printf("Invalid parameters.\n");
                return EXIT_FAILURE;
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

    Nerd *nerd = nerd_constructor(TRAIN, ',', false, true, 10.0, 3, 1, 10, 1, 9, true);

    nerd_start_learning(nerd);

    nerd_destructor(&nerd);

    remove(TEST);
    remove(TRAIN);

    return EXIT_SUCCESS;
}
