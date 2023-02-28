#include <stdio.h>
#include <string.h>

#include "scene.h"

/**
 * @brief Constructs a Scene.
 *
 * @return A new Scene *. Use scene_destructor to deallocate.
 */
Scene *scene_constructor() {
    Scene *scene = (Scene *) malloc(sizeof(Scene));
    scene->literals = NULL;
    scene->size = 0;
    return scene;
}

/**
 * @brief Destructs a Scene. If NULL is given, nothing will happen.
 *
 * @param scene The Scene to be destructed. It should be a reference to the object's pointer.
 */
void scene_destructor(Scene ** const scene) {
    if (scene && (*scene)) {
        if ((*scene)->literals) {
            unsigned int i = 0;
            for (i = 0; i < (*scene)->size; ++i) {
                (*scene)->literals[i] = NULL;
            }

            free((*scene)->literals);
            (*scene)->literals = NULL;
            (*scene)->size = 0;
        }
        free(*scene);
        *scene = NULL;
    }
}

/**
 * @brief Makes a copy of the given Scene.
 *
 * @param destination The Scene to save the copy. It should be a reference to the object's pointer.
 * @param source The Scene to be copied. If Scene or its literals are NULL, the content of the
 * destination will not be changed.
 */
void scene_copy(Scene ** const destination, const Scene * const restrict source) {
    if (destination && source) {
        *destination = scene_constructor();

        if (source->size == 0) {
            return;
        }

        (*destination)->size = source->size;
        (*destination)->literals = (Literal **) malloc(source->size * sizeof(Literal *));

        unsigned int i;
        for (i = 0; i < source->size; ++i) {
            (*destination)->literals[i] = source->literals[i];
        }
    }
}

/**
 * @brief Adds a Literal to the Scene. It does not acquire ownership, so if the given pointer gets
 * deallocated, there will be a segmentation fault.
 *
 * @param scene The Scene to be expanded.
 * @param literal_to_add The Literal to add in the Scene. It should be a Literal *.
 */
void scene_add_literal(Scene * const scene, Literal * const literal_to_add) {
    if (scene && literal_to_add) {
        if (scene_literal_index(scene, literal_to_add) == -1) {
            ++scene->size;
            scene->literals = (Literal **) realloc(scene->literals,
            scene->size * sizeof(Literal *));
            scene->literals[scene->size - 1] = literal_to_add;
        }
    }
}

/**
 * @brief Removes a Literal from a Scene.
 *
 * @param scene The Scene to be reduced.
 * @param literal_index The index of the Literal to be removed.
 */
void scene_remove_literal(Scene * const scene, const unsigned int literal_index) {
    if (scene) {
        if (literal_index < scene->size) {
            scene->literals[literal_index] = NULL;
            --scene->size;
            Literal **literals = scene->literals;
            scene->literals = (Literal **) malloc(scene->size * sizeof(Literal *));
            if (literal_index == 0) {
                memcpy(scene->literals, literals + 1, scene->size * sizeof(Literal *));
            } else if (literal_index == (scene->size + 1)) {
                memcpy(scene->literals, literals, scene->size * sizeof(Literal *));
            } else {
                memcpy(scene->literals, literals, literal_index * sizeof(Literal *));
                memcpy(scene->literals + literal_index, literals + literal_index + 1,
                (scene->size - literal_index) * sizeof(Literal *));
            }
            free(literals);
        }
    }
}

/**
 * @brief Finds the index of the given Literal in the Scene.
 *
 * @param scene The Scene to find the Literal.
 * @param literal The Literal to be found.
 *
 * @return the index of the Literal in the Scene, -1 if it doesn't exist or -2 if the Literal, the
 * Scene or both are NULL.
 */
int scene_literal_index(const Scene * const scene, const Literal * const literal) {
    if (scene && literal) {
        unsigned int i;
        for (i = 0; i < scene->size; ++i) {
            if (literal_equals(literal, scene->literals[i])) {
                return i;
            }
        }
        return -1;
    }
    return -2;
}

/**
 * @brief Performs a set union operation on two Scenes (scene1 ∪ scene2).
 *
 * @param scene1 The first Scene to be combined.
 * @param scene2 The second Scene to be combined.
 * @param result The output of the operation to be returned. If NULL, the operation will not be
 * performed. It should be a reference to the object's pointer.
 */
void scene_union(const Scene * const restrict scene1, const Scene * const restrict scene2,
Scene ** const restrict result) {
    if (result) {
        if (scene1) {
            scene_copy(result, scene1);
            if (scene2 && (scene2->size != 0)) {
                unsigned int i, j;
                for (i = 0; i < scene2->size; ++i) {
                    for (j = 0; j < scene1->size; ++j) {
                        if (literal_equals(scene2->literals[i], scene1->literals[j])) {
                            break;
                        }
                    }

                    if (j == scene1->size) {
                        scene_add_literal_copy(*result, scene2->literals[i]);
                    }
                }
            }
        } else if (scene2) {
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
 * performed. It should be a reference to the object's pointer.
 */
void scene_difference(const Scene * const restrict scene1, const Scene * const restrict scene2,
Scene ** const restrict result) {
    if (result) {
        if (scene1) {
            if (scene2 && (scene2->size != 0)) {
                *result = scene_constructor();

                unsigned int i, j;
                for (i = 0; i < scene1->size; ++i) {
                    for (j = 0; j < scene2->size; ++j) {
                        if (literal_equals(scene2->literals[j], scene1->literals[i])) {
                            break;
                        }
                    }

                    if (j == scene2->size) {
                        scene_add_literal_copy(*result, scene1->literals[i]);
                    }
                }
            } else {
                scene_copy(result, scene1);
            }
        } else if (scene2) {
            scene_copy(result, scene2);
        }
    }
}

/**
 * @brief Performs a set intersection operation on two Scenes (scene1 ⋂ scene2).
 *
 * @param scene1 The first Scene to compare.
 * @param scene2 The second Scene to compare.
 * @param result The output of the operation to be returned. If NULL, the operation will not be
 * performed. It should be a reference to the object's pointer.
 */
void scene_intersect(const Scene * const restrict scene1, const Scene * const restrict scene2,
Scene ** const restrict result) {
    if (scene1 && scene2 && result) {
        *result = scene_constructor();

        unsigned int i, j;
        for (i = 0; i < scene1->size; ++i) {
            for (j = 0; j < scene2->size; ++j) {
                if (literal_equals(scene1->literals[i], scene2->literals[j])) {
                    scene_add_literal_copy(*result, scene1->literals[i]);
                    break;
                }
            }
        }
    }
}

// TODO Add implementation or remove.
/**
 * @brief Performs a set subset check on two Scenes (scene1 ⊆ scene2).
 *
 * @param scene1 The Scene to be checked.
 * @param scene2 The Scene which scene1 is to be checked with.
 *
 * @return 1 if scene1 ⊆ scene2, 0 if scene1 ⊈ scene2, or -1 if one of the Scenes was NULL.
*/
void scene_subset() {

}

// TODO Add comment.
int scene_number_of_similar_literals(const Scene * const restrict scene1,
const Scene * const scene2) {
    if (!(scene1 && scene2)) {
        return -1;
    }

    unsigned int i, j, total_equals = 0;
    for (i = 0; i < scene1->size; ++i) {
        for (j = 0; j < scene2->size; ++j) {
            if (literal_equals(scene1->literals[i], scene2->literals[j]) == 1) {
                ++total_equals;
                break;
            }
        }
    }

    return total_equals;
}

int scene_number_of_opposed_literals(const Scene * const restrict scene1,
const Scene * const restrict scene2) {
    if (!(scene1 && scene2)) {
        return -1;
    }

    unsigned int i, j, total_opposed = 0;
    for (i = 0; i < scene1->size; ++i) {
        for (j = 0; j <scene2->size; ++j) {
            if (literal_opposed(scene1->literals[i], scene2->literals[j]) == 1) {
                ++total_opposed;
                break;
            }
        }
    }
    return total_opposed;
}

/**
 * @brief Finds all the literals that have the same atom, but a different (opposing) sign. If any of
 *  the parameters is NULL, the operation will not be performed.
 *
 * @param scene1 The Scene used as ground truth.
 * @param scene2 The Scene to find the different Literals.
 * @param result The output of the operation to be returned. If NULL, the operation will not be
 * performed. It should be a reference to the object's pointer.
 */
void scene_opposed_literals(const Scene * const restrict scene1,
const Scene * const restrict scene2, Scene ** const restrict result) {
    if (scene1 && scene2 && result) {
        *result = scene_constructor();
        unsigned int i, j;
        for (i = 0; i < scene1->size; ++i) {
            for (j = 0; j < scene2->size; ++j) {
                if (!literal_equals(scene1->literals[i], scene2->literals[j])) {
                    if (strcmp(scene1->literals[i]->atom, scene2->literals[j]->atom) == 0) {
                        scene_add_literal_copy(*result, scene2->literals[j]);
                        break;
                    }
                } else {
                    break;
                }
            }
        }
    }
}

/**
 * @brief Converts a Scene into a string format.
 *
 * @param scene The Scene to be converted.
 *
 * @return The string format of the given Scene. Use free() to deallocate this string. Returns NULL 
 * if the Scene is NULL.
 */
char *scene_to_string(const Scene * const scene) {
    if (scene) {
        if (scene->size == 0) {
            return strdup("Scene: [\n]");
        }
        char *result, *temp, *literal_string, *scene_string = "Scene: [\n";
        size_t result_size = strlen(scene_string) + 1;

        result = strdup(scene_string);

        unsigned int i;
        for (i = 0; i < scene->size - 1; ++i) {
            literal_string = literal_to_string(scene->literals[i]);
            temp = strdup(result);
            result_size += strlen(literal_string) + 3;
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s\t%s,\n", temp, literal_string);

            free(temp);
            free(literal_string);
        }

        literal_string = literal_to_string(scene->literals[i]);
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
