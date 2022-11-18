#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "context.h"

/**
 * @brief Constructs a Context. Alias for scene_constructor.
 * 
 * @return A new Context *. Use context_destructor to deallocate.
 */
Context *context_constructor() {
    Context *context = NULL;
    context = scene_constructor();
    return context;
}

/**
 * @brief Destructs a Context. Alias for scene_constructor.
 * 
 * @param context The context to be destructed. It should be a reference to the object's pointer.
 */
void context_destructor(Context **context) {
    scene_destructor(context);
}

/**
 * @brief Adds a Literal to a Context. Alias for a scene_add_literal.
 */
void context_add_literal(Context * const context, const Literal * const literal) {
    scene_add_literal(context, literal);
}

/**
 * @brief Converts a Context to a Prudens JS Context format.
 * 
 * @param context The Context to be converted.
 *
 * @return The Prudens JS Context format (as a string) of the given Context. Use free() to 
 * deallocate the result. Returns NULL if the Context or its body are NULL.
 */
char *context_to_prudensjs(const Context * const context) {
    if (context) {
        if (context->observations) {
            char *result = strdup("{\"type\": \"output\", \"context\": ["), *temp,
            *literal_prudensjs_string;
            size_t result_size = strlen(result) + 1;

            unsigned int i;
            for (i = 0; i < context->size - 1; ++i) {
                literal_prudensjs_string = literal_to_prudensjs(context->observations[i]);
                temp = strdup(result);
                result_size += strlen(literal_prudensjs_string) + 2;
                result = (char *) realloc(result, result_size);
                sprintf(result, "%s%s, ", temp, literal_prudensjs_string);
                free(temp);
                free(literal_prudensjs_string);
            }

            literal_prudensjs_string = literal_to_prudensjs(context->observations[i]);
            temp = strdup(result);
            result_size += strlen(literal_prudensjs_string) + 2;
            result = (char *) realloc(result, result_size);
            sprintf(result, "%s%s]}", temp, literal_prudensjs_string);
            free(temp);
            free(literal_prudensjs_string);

            return result;
        }
    }
    return NULL;
}