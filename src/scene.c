#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scene.h"

/**
 * @brief Constructs a Scene. If NULL is given, nothing will happen.
 * 
 * @param scene The Scene to be constructed.
 */
void scene_constructor(Scene * const scene) {
    if (scene != NULL) {
        scene->observations = NULL;
        scene->size = 0;
    }
}

/**
 * @brief Destructs a Scene. If NULL is given, nothing will happen.
 * 
 * @param scene The Scene to be destructed.
 */
void scene_destructor(Scene * const scene) {
    if (scene != NULL) {
        if (scene->observations != NULL) {
            unsigned int i = 0;
            for (i = 0; i < scene->size; ++i) {
                literal_destructor(&(scene->observations[i]));
            }

            free(scene->observations);
            scene->observations = NULL;
            scene->size = 0;
        }
    }
}

/**
 * @brief Makes a copy of the given Scene.
 * 
 * @param destination The Scene to save the copy.
 * @param source The Scene to be copied. If Scene or its observations are NULL, the content of the 
 * destination will not be changed.
 */
void scene_copy(Scene * const destination, const Scene * const source) {
    if ((destination != NULL) && (source != NULL)) {
        destination->size = source->size;
        destination->observations = (Literal *) malloc(source->size * sizeof(Literal));
        
        unsigned int i;
        for (i = 0; i < source->size; ++i) {
            literal_copy(&(destination->observations[i]), &(source->observations[i]));
        }
    }
}

/**
 * @brief Adds a Literal to the Scene.
 * 
 * @param scene The Scene to be expanded.
 * @param literal_to_add The Literal to added in the Scene.
 */
void scene_add_literal(Scene * const scene, const Literal * const literal_to_add) {
    if ((scene != NULL) && (literal_to_add != NULL)) {
        ++scene->size;
        scene->observations = (Literal *) realloc(scene->observations,
        scene->size * sizeof(Literal));
        literal_copy(&(scene->observations[scene->size - 1]), literal_to_add);
    }
}

/**
 * @brief Converts a Scene into a string format.
 * 
 * @param scene The Scene to be converted.
 * @return The string format of the given Scene. Use free() to deallocate this string. Returns NULL 
 * if the Scene is NULL.
 */
char *scene_to_string(const Scene * const scene) {
    if (scene != NULL) {
        if (scene->size == 0) {
            return strdup("Scene: [\n]");
        }
        char *result, *temp, *literal_string, *scene_string = "Scene: [\n";
        size_t result_size = strlen(scene_string) + 1;

        result = strdup(scene_string);

        unsigned int i;
        for (i = 0; i < scene->size - 1; ++i) {
            literal_string = literal_to_string(&(scene->observations[i]));
            temp = strdup(result);
            result_size += strlen(literal_string) + 3;
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s\t%s,\n", temp, literal_string);

            free(temp);
            free(literal_string);
        }

        literal_string = literal_to_string(&(scene->observations[i]));
        temp = strdup(result);
        result_size += strlen(literal_string) + 3;
        result = (char *) realloc(result, result_size);
        sprintf(result, "%s\t%s\n]", temp, literal_string);

        free(temp);
        free(literal_string);

        return result;
    }
    return NULL;
}