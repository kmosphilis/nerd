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
 * @param context The context to be destructed. It should be a reference to the struct's pointer (to
 * a Context *).
 */
void context_destructor(Context ** const context) {
    scene_destructor(context);
}

/**
 * @brief Adds a Literal to the Context by taking its ownership. Alias for a scene_add_literal.
 *
 * @param context The Context to be expanded.
 * @param literal_to_add The Literal to add in the Context. It should be a reference to a Literal *
 * (Literal ** - a pointer to a Literal *). Upon succession, this parameter will become NULL.
 */
void context_add_literal(Context * const context, Literal ** const literal_to_add) {
    scene_add_literal(context, literal_to_add);
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
        if (context->literals) {
            char *result = strdup("{\"type\": \"output\", \"context\": ["), *temp,
            *literal_prudensjs_string;
            size_t result_size = strlen(result) + 1;

            unsigned int i;
            for (i = 0; i < context->size - 1; ++i) {
                literal_prudensjs_string = literal_to_prudensjs(context->literals[i]);
                temp = strdup(result);
                result_size += strlen(literal_prudensjs_string) + 2;
                result = (char *) realloc(result, result_size);
                sprintf(result, "%s%s, ", temp, literal_prudensjs_string);
                free(temp);
                free(literal_prudensjs_string);
            }

            literal_prudensjs_string = literal_to_prudensjs(context->literals[i]);
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
