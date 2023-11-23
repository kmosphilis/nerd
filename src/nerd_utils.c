#include <string.h>
#include <ctype.h>
#include <math.h>

#include "nerd_utils.h"

/**
 * @brief safe_free macro implementation.
*/
void _safe_free(void **ptr) {
    free(*ptr);
    *ptr = NULL;
}

/**
 * @brief Removes all the whitespaces from both ends of the given string.
 *
 * @param string The string to filter.
 *
 * @return A newly allocated trimmed string, or NULL if string is whitespaces only. Use free to
 * deallocate.
*/
char *trim(const char * const string) {
    if (!string || (strlen(string) == 0)) {
        return NULL;
    }

    char *trimmed_string = strdup(string), *temp;
    size_t length = strlen(trimmed_string) + 1;

    while (isspace(trimmed_string[0])) {
        temp = trimmed_string;
        trimmed_string = (char *) malloc(--length * sizeof(char));
        memcpy(trimmed_string, temp + 1, length * sizeof(char));
        free(temp);
    }

    while (isspace(trimmed_string[length - 2])) {
        trimmed_string[length - 2] = '\0';
        trimmed_string = realloc(trimmed_string, --length * sizeof(char));
    }
    return trimmed_string;
}

int _compare(const void *a, const void *b) {
    return (*(int *) a - *(int *) b);
}

/**
 * @brief Splits the given dataset into a train and a test streams.
 *
 * @param dataset The FILE pointer to the initial dataset.
 * @param has_header Boolean value which indicates whether the given dataset has boolean values or
 * not.
 * @param test_ratio A float indicating the ratio of the testing dataset, given the dataset's size.
 * @param generator A pcg32_random_t pointer to an RNG.
 * @param train_path A string containing the filename location to save the training dataset. If
 * NULL, the train will contain a tmpfile.
 * @param test_path A string containing the filename location to save the testing dataset. If NULL,
 * the test parameter will contain a tmpfile.
 * @param train A double pointer - reference to a FILE * - &(FILE *) - to save the training dataset.
 * If NULL, the training split will not be saved.
 * @param test A double pointer - reference to a FILE * - &(FILE *) - to save the testing dataset.
 * If NULL, the testing split will not be saved.
 *
 * @return 0 if the process was successful, -1 if the given train_path was incorrect and the train
 * dataset was saved in a tmpfile, -2 if the given test_path was incorrect and the test dataset was
 * saved in a tmpfile, or -3 if both were incorrect.
*/
int train_test_split(FILE *dataset, const bool has_header, const float test_ratio,
pcg32_random_t *generator, const char * const train_path, const char * const test_path,
FILE **train, FILE **test) {
    FILE *train_ = NULL, *test_ = NULL;
    int error_code = 0;

    if (train_path) {
        train_ = fopen(train_path, "wb+");
        if (!train_) {
            error_code = -1;
            train_ = tmpfile();
        }
    } else {
        train_ = tmpfile();
    }

    if (test_path) {
        test_ = fopen(test_path, "wb+");
        if (!test_) {
            error_code += -2;
            test_ = tmpfile();
        }
    } else {
        test_ = tmpfile();
    }

    size_t dataset_size = 0;
    int c;
    if (has_header) {
        do {
            c = fgetc(dataset);
            fputc(c, train_);
            fputc(c, test_);
        } while (c != '\n');
    }
    fpos_t beginning;
    fgetpos(dataset, &beginning);

    for (c = fgetc(dataset); c != EOF; c = fgetc(dataset)) {
        if (c == '\n') {
            ++dataset_size;
        }
    }
    fsetpos(dataset, &beginning);

    fpos_t *instances = (fpos_t *) malloc((dataset_size + 1) * sizeof(fpos_t));

    unsigned int i = 0;
    instances[i++] = beginning;

    do {
        c = fgetc(dataset);
        if (c == '\n') {
            fgetpos(dataset, &(instances[i++]));
        }
    } while (c != EOF);

    size_t test_size = roundf(dataset_size * test_ratio);
    unsigned int *possible_indices = (unsigned int *) malloc(dataset_size * sizeof(int));

    for (i = 0; i < dataset_size; ++i) {
        possible_indices[i] = i;
    }

    int chosen_index;
    size_t remaining = dataset_size;
    for (i = 0; i < test_size; ++i) {
        chosen_index = pcg32_random_r(generator) % remaining--;
        fsetpos(dataset, &(instances[possible_indices[chosen_index]]));
        do {
            c = fgetc(dataset);
            fputc(c, test_);
        } while (c != '\n');
        possible_indices[chosen_index] = possible_indices[remaining];
    }

    for (i = 0; i < (dataset_size - test_size); ++i) {
        chosen_index = pcg32_random_r(generator) % remaining--;
        fsetpos(dataset, &(instances[possible_indices[chosen_index]]));
        do {
            c = fgetc(dataset);
            fputc(c, train_);
        } while (c != '\n');
        possible_indices[chosen_index] = possible_indices[remaining];
    }

    free(instances);
    free(possible_indices);

    if (train) {
        *train = train_;
    } else {
        fclose(train_);
    }

    if (test) {
        *test = test_;
    } else {
        fclose(test_);
    }

    return error_code;
}

/* IntVector implementation. */

/**
 * @brief Constructs an IntVector.
 *
 * @return A new IntVector *. Use int_vector_destructor to deallocate.
 */
IntVector *int_vector_constructor() {
    IntVector *int_vector = (IntVector *) malloc(sizeof(IntVector));
    int_vector->items = NULL;
    int_vector->size = 0;
    return int_vector;
}

/**
 * @brief Destructs an IntVector.
 *
 * @param int_vector The IntVector to be destructed. If NULL the process will fail. It should be a
 * reference to the object's pointer.
 */
void int_vector_destructor(IntVector ** const int_vector) {
    if (int_vector && (*int_vector)) {
        if ((*int_vector)->items) {
            safe_free((*int_vector)->items);
            (*int_vector)->size = 0;
        }
        safe_free(*int_vector);
    }
}

/**
 * @brief Makes a copy of the given IntVector.
 *
 * @param destination The IntVector to save the copy.
 * @param source The IntVector to be copied. If NULL is given, the contents of the destination will
 * not be changed.
 */
void int_vector_copy(IntVector ** const destination, const IntVector * const restrict source) {
    if (destination && source) {
        *destination = int_vector_constructor();
        if (source->items) {
            (*destination)->items = (int *) malloc(source->size * sizeof(int));
            memcpy((*destination)->items, source->items, source->size * sizeof(int));
            (*destination)->size = source->size;
        }
    }
}

/**
 * @brief Resizes an IntVector. If new_size > int_vector.size, the new elements will be set to 0;
 *
 * @param int_vector The IntVector to be resized. If NULL the process will fail.
 * @param new_size The new size of the vector.
 */
void int_vector_resize(IntVector * const int_vector, const unsigned int new_size) {
    if (int_vector) {
        if (new_size == 0) {
            int_vector->size = 0;
            safe_free(int_vector->items);
            return;
        }

        unsigned int old_size = int_vector->size;
        int *old = int_vector->items;

        int_vector->items = (int *) malloc(new_size * sizeof(int));
        int_vector->size = new_size;

        if (new_size > old_size) {
            memcpy(int_vector->items, old, sizeof(int) * old_size);
        } else {
            memcpy(int_vector->items, old, sizeof(int) * new_size);
        }

        free(old);

        unsigned int i;
        for (i = old_size; i < new_size; ++i) {
            int_vector->items[i] = 0;
        }
    }
}

/**
 * @brief Pushes a new item at the back of the IntVector.
 *
 * @param int_vector The IntVector to push the new element in. If NULL the process will fail.
 * @param item The item to be added in the IntVector.
 */
void int_vector_push(IntVector * const int_vector, const int item) {
    if (int_vector) {
        int_vector_resize(int_vector, int_vector->size + 1);
        int_vector->items[int_vector->size - 1] = item;
    }
}

/**
 * @brief Expands the IntVector by adding a new item at the given position. The item will be added
 * to that position, and all subsequent values will have an index = old index + 1. If the given
 * position is equal to the size of the vector, the element will function like int_vector_push().
 *
 * @param int_vector The IntVector to add the new element in. If NULL the process will fail.
 * @param position The position to add the item to.
 * @param item The item to be added in the IntVector.
 */
void int_vector_insert(IntVector * const int_vector, const unsigned int position, const int item) {
    if (int_vector) {
        if (int_vector->size < position) {
            if (position == int_vector->size) {
                int_vector_push(int_vector, item);
            }
            return;
        }

        int *old_items = int_vector->items;
        int_vector->items = (int *) malloc((int_vector->size + 1) * sizeof(int));

        if (position == 0) {
            int_vector->items[0] = item;
            memcpy(int_vector->items + 1, old_items, int_vector->size * sizeof(int));
        } else {
            memcpy(int_vector->items, old_items, position * sizeof(int));
            int_vector->items[position] = item;
            memcpy(int_vector->items + position + 1, old_items + position,
            (int_vector->size - position) * sizeof(int));
        }
        ++int_vector->size;
        free(old_items);
    }
}

/**
 * @brief Deletes the item at the given index and resize the vector.
 *
 * @param int_vector The IntVector to delete the item from. If NULL the process will fail.
 * @param index The index of the item to be removed. If index > int_vector.size, the process will
 * fail.
 */
void int_vector_delete(IntVector * const int_vector, const unsigned int index) {
    if (int_vector) {
        if (int_vector->size > index) {
            unsigned int i;
            for (i = index; i < int_vector->size - 1; ++i) {
                int_vector->items[i] = int_vector->items[i + 1];
            }
            --int_vector->size;
            int_vector_resize(int_vector, int_vector->size);
        }
    }
}

/**
 * @brief Retrieves the item from the given index position. Equivalent to int_vector.items[index]
 * but with checks.
 *
 * @param int_vector The IntVector to retrieve the item from. If NULL the process will fail.
 * @param index The index of the required item. If index > int_vector.size, the process will fail.
 *
 * @return The item (int) at the given index, or the index if out of bounds or int_vector is NULL.
 */
int int_vector_get(const IntVector * const int_vector, const unsigned int index) {
    if (int_vector) {
        if (int_vector->size > index) {
            return int_vector->items[index];
        }
        return index;
    }
    return index;
}

/**
 * @brief Sets the given item (int) to the given index. Equivalent to int_vector.items[index] =
 * new_item but with checks.
 *
 * @param int_vector The IntVector to set the new item to. If NULL the process will fail.
 * @param index The index of the item to be replaces. If index > int_vector.size, the process will
 * fail.
 * @param new_item The new item to be set at the given index.
 *
 * @return 1 if successful, 0 if out of bounds, or -1 if int_vector is NULL.
 */
int int_vector_set(IntVector * const int_vector, const unsigned int index, const int new_item) {
    if (int_vector) {
        if (int_vector->size > index) {
            int_vector->items[index] = new_item;
            return 1;
        }
        return 0;
    }
    return -1;
}
