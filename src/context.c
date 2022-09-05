#include "context.h"

/**
 * @brief Constructs a Context. Alias for scene_constructor.
 * 
 * @param context The context to be constructed.
 */
void context_constructor(Context * const context) {
    scene_constructor((Scene *) context);
}

/**
 * @brief Destructs a Context. Alias for scene_constructor.
 * 
 * @param context The context to be destructed.
 */
void context_destructor(Context * const context) {
    scene_destructor(context);
}

/**
 * @brief Adds a Literal to a Context. Alias for a scene_add_literal.
 */
void context_add_literal(Context * const context, const Literal * const literal) {
    scene_add_literal(context, literal);
}