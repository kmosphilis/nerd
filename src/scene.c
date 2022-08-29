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
 * @param literal_to_add The Literal to add in the Scene.
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
 * @brief Performs a set union operation on two Scenes (scene1 ∪ scene2).
 * 
 * @param scene1 The first Scene to be combined.
 * @param scene2 The second Scene to be combined.
 * @param result The output of the operation to be returned. If NULL, the operation will not be 
 * performed.
 */
void scene_combine(const Scene * const scene1, const Scene * const scene2, Scene * const result) {
    if (result != NULL) {
        if (scene1 != NULL) {
            scene_copy(result, scene1);
            if (scene2 != NULL) {
                unsigned int i, j;
                for (i = 0; i < scene2->size; ++i) {
                    for (j = 0; j < scene1->size;++j) {
                        if (literal_equals(&(scene2->observations[i]),
                        &(scene1->observations[j]))) {
                            break;
                        }
                    }
                    
                    if (j == scene1->size) {
                        scene_add_literal(result, &(scene2->observations[i]));
                    }
                }
            }
        } else if (scene2 != NULL) {
            scene_copy(result, scene2);
        }
    }
}

/**
 * @brief Performs a set difference operation on two Scenes (scene1 ∖ scene2).
 * 
 * @param scene1 The Scene to remove elements from.
 * @param scene2 The Scene to compare with.
 * @param result The output of the operation to be returned. If NULL, the operation will not be 
 * performed.
 */
void scene_difference(const Scene * const scene1, const Scene * const scene2, Scene * const result) {
    if (result != NULL) {
        if (scene1 != NULL) {
            if (scene2 != NULL) {
                unsigned int i, j;
                for (i = 0; i < scene1->size; ++i) {
                    for (j = 0; j < scene2->size; ++j) {
                        if (literal_equals(&(scene2->observations[i]),
                        &(scene1->observations[j]))) {
                            break;
                        }
                    }

                    if (j == scene2->size) {
                        scene_add_literal(result, &(scene1->observations[i]));
                    }
                }
            } else {
                scene_copy(result, scene1);
            }
        } else if (scene2 != NULL) {
            scene_copy(result, scene2);
        }
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