#include <string.h>

#include "int_vector.h"

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
            free((*int_vector)->items);
            (*int_vector)->items = NULL;
            (*int_vector)->size = 0;
        }
        free(*int_vector);
        *int_vector = NULL;
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
            free(int_vector->items);
            int_vector->items = NULL;
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
