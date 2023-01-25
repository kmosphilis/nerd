#include <stdlib.h>

#include "metrics.h"

/**
 * @brief Evaluates whether Nerd's KnowledgeBase can predict all the observed Literals when hidding
 * a Literal. If the given observation has 6 Literals, 6 different scenarios will be tested
 * accordingly.
 *
 * @param nerd The Nerd struct where the learnt KnowledgeBase to evaluate is.
 * @param scene The scene to evaluate the learnt KnowledgeBase with.
 * @param overall_success Pointer to save the overall success of KnowledbeBase to predict the
 * observation. It can vary from [0, 1].
*/
void evaluate_all_literals(const Nerd * const nerd, const Scene * const observation,
float * const overall_success) {
    if (!(observation && nerd)) {
        return;
    }
    Scene test_scene, inferred;
    float success = 0;
    unsigned int i;

    scene_constructor(&test_scene);
    scene_constructor(&inferred);

    for (i = 0; i < observation->size; ++i) {
        scene_copy(&test_scene, observation);
        scene_remove_literal(&test_scene, i);

        prudensjs_inference(&(nerd->knowledge_base), &test_scene, &inferred);
        
        success += scene_number_of_similar_literals(&inferred, observation) /
        (float) observation->size;
        scene_destructor(&test_scene);
        scene_destructor(&inferred);
    }

    scene_destructor(&test_scene);
    scene_destructor(&inferred);
    *overall_success = success / observation->size;
}

// TODO Rethink.
// void evaluate_hidden_literals(const Nerd * const nerd, const Scene * const observation,
// const float ratio_of_hidden_literals, float * const overall_success) {
//     if (!(nerd && observation && (ratio_of_hidden_literals >= 0.0) && 
//     (ratio_of_hidden_literals < 1.0))) {
//         return;
//     }
//     Scene test_scene, inferred;
//     unsigned int i;
//     int literal_to_hide = (int) (observation->size * ratio_of_hidden_literals);
//     if (literal_to_hide == 0) {
//         ++literal_to_hide;
//     }

//     scene_constructor(&test_scene);
//     scene_constructor(&inferred);
//     scene_copy(&test_scene, observation);

//     for (i = 0; i < literal_to_hide; ++i) {
//         scene_remove_literal(&test_scene, rand() % test_scene.size);
//     }

//     prudensjs_inference(&(nerd->knowledge_base), &test_scene, &inferred);
//     *overall_success = scene_number_of_similar_literals(&inferred, observation) /
//     (float) observation->size;

//     scene_destructor(&test_scene);
//     scene_destructor(&inferred);
// }
