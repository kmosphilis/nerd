#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "nerd.h"
#include "metrics.h"

/**
 * @brief Constructs a Nerd structure (object).
 *
 * @param nerd The Nerd structure to be constructed.
 * @param filepath The path to the file containing the learning instances for the Nerd algorithm.
 * @param reuse Specifies whether to reuse the stream from the beginning when it reaches the EOF. 
 * Use > 0 to indicate yes, and <= 0 to indicate no. In the case that it should not reuse the file, 
 * the algorithm will ignore the given epochs.
 * @param activation_threshold The threshold which determines whether a rule should get activated 
 * or deactivated.
 * @param breadth The maximum size of Literals that a Rule should contain. //FIXME What if zero is given?
 * @param depth The maximum size of the KnowledgeBase //FIXME Currently not being used.
 * @param epochs The number of epochs the algorithm should learn for.
 * @param promotion_weight The amount that a Rule should be promoted with. It should be > 0.
 * @param demotion_weight The amount that a Rule should be demoted with. It should be > 0.
 * @param partial_observation If partial_observation is > 0, the initial observation will be saved 
 * here. If NULL is given, it will not be saved.
 */
void nerd_constructor(Nerd * const nerd, const char * const filepath, unsigned short reuse,
const float activation_threshold, const unsigned int breadth, const unsigned int depth,
const unsigned int epochs, const float promotion_weight, const float demotion_weight,
const unsigned short partial_observation) {
    if (nerd && filepath) {
        sensor_constructor_from_file(&(nerd->sensor), filepath, reuse);
        nerd->breadth = breadth;
        nerd->depth = depth;
        nerd->epochs = epochs;
        nerd->promotion_weight = promotion_weight;
        nerd->demotion_weight = demotion_weight;
        knowledge_base_constructor(&(nerd->knowledge_base), activation_threshold);
        nerd->partial_observation = partial_observation;
    }
}

/**
 * @brief Destructs a Nerd structure (object).
 *
 * @param nerd The Nerd structure to be destructed.
 */
void nerd_destructor(Nerd * const nerd) {
    if (nerd) {
        nerd->breadth = 0;
        nerd->depth = 0;
        nerd->epochs = 0;
        nerd->promotion_weight = INFINITY;
        nerd->demotion_weight = INFINITY;
        sensor_destructor(&(nerd->sensor));
        knowledge_base_destructor(&(nerd->knowledge_base));
        nerd->partial_observation = 0;
    }
}

// TODO Add comment
void prudensjs_inference(const KnowledgeBase * const knowledge_base,
const Scene * const restrict observation, Scene * restrict inferred) {
    const char * const temp_filename = ".temp";
    char *knowledge_base_prudensjs = knowledge_base_to_prudensjs(knowledge_base),
    *context_prudensjs = context_to_prudensjs(observation);
    if (!(knowledge_base_prudensjs && context_prudensjs)) {
        return;
    }

    FILE *file = fopen(temp_filename, "wb");

    fprintf(file, "%s\n%s", knowledge_base_prudensjs, context_prudensjs);
    free(knowledge_base_prudensjs);
    free(context_prudensjs);
    fclose(file);
    system("node prudens-infer.js");

    file = fopen(temp_filename, "rb");
    if (feof(file)) {
        fclose(file);
        return;
    }

    char buffer[BUFFER_SIZE];
    while (fscanf(file, "%s", buffer) != EOF) {
        Literal literal;
        if (buffer[0] == '-') {
            literal_constructor(&literal, buffer + 1, 0);
        } else {
            literal_constructor(&literal, buffer, 1);
        }
        scene_add_literal(inferred, &literal);
        literal_destructor(&literal);
    }

    fclose(file);
    remove(temp_filename);
}

/**
 * @brief Initiates the learning.
 *
 * @param nerd The Nerd structure containing all the info for learn new Rules.
 */
void nerd_start_learning(Nerd * const nerd) {
    if (!nerd) {
        return;
    }
    /*  1 - Create rules with heads that do not exist in effective rules (rules that were applicable and were correct).
        2 - Promote rules applicable on inferred that concur with observed. Promote both active and inactive rules.
            If head is the same as observed, promote, else demote it if it opposed.
        3 - Weak priority (it is implemented by default).
        4 - Demote applicable rules that oppose observed literals.
        5 - No priority for newly inactive rules (implemented by default).
    */

    Scene observation, observed_and_inferred, inferred;
    unsigned int epoch, iteration, index, k, j;
    const size_t total_observations = sensor_get_total_observations(&(nerd->sensor));

    scene_constructor(&observation);
    scene_constructor(&observed_and_inferred);
    scene_constructor(&inferred);

    char *str = knowledge_base_to_string(&(nerd->knowledge_base));
    printf("%s\n", str);
    free(str);
    time_t start = time(NULL);

    for (epoch = 0; epoch < nerd->epochs; ++epoch) {
        for (iteration = 0; iteration < total_observations; ++iteration) {
            printf("\nEpoch %d of %d, Iteration %d of %ld\n", epoch + 1, nerd->epochs,
            iteration + 1, total_observations);
            sensor_get_next_scene(&(nerd->sensor), &observation, nerd->partial_observation, NULL);

            prudensjs_inference(&(nerd->knowledge_base), &observation, &inferred);
            scene_union(&observation, &inferred, &observed_and_inferred);

            knowledge_base_create_new_rules(&(nerd->knowledge_base), &observation, &inferred,
            nerd->breadth, 5);
            Scene uncovered;
            scene_constructor(&uncovered);
            scene_difference(&observation, &inferred, &uncovered);

            str = scene_to_string(&observation);
            printf("Observed: %s\n", str);
            free(str);
            str = scene_to_string(&inferred);
            printf("Inferred: %s\n", str);
            free(str);
            str = scene_to_string(&uncovered);
            printf("Observed ∖ Inferred: %s\n", str);
            free(str);

            unsigned int rules_demoted = 0, rules_promoted = 0, rules_deleted = 0;
            IntVector active_rules_to_demote, demoted_applicable_rules, applicable_rules;
            int_vector_constructor(&active_rules_to_demote);
            int_vector_constructor(&demoted_applicable_rules);
            int_vector_constructor(&applicable_rules);

            for (index = nerd->knowledge_base.active.length; index > 0; --index) {
                const Rule * const current_rule = &(nerd->knowledge_base.active.rules[index - 1]);

                if (rule_applicable(current_rule, &observed_and_inferred)) {
                    int_vector_push(&applicable_rules, index - 1);
                }
            }

            for (index = 0; index < applicable_rules.size; ++index) {
                const unsigned int current_rule_index = int_vector_get(&applicable_rules, index);
                const Rule * const current_rule = &(nerd->knowledge_base.active
                .rules[current_rule_index]);

                int rule_concurs_result = rule_concurs(current_rule, &observation);
                if (rule_concurs_result == 1) {
                    knowledge_base_promote_rule(&(nerd->knowledge_base), ACTIVE, current_rule_index,
                    nerd->promotion_weight);
                } else if (rule_concurs_result == 0) {
                    int opposed_to_observation = 0;
                    for (k = 0 ; k < uncovered.size; ++k) {
                        if (literal_opposed(&(current_rule->head),
                        &(uncovered.observations[k])) == 1) {
                            opposed_to_observation = 1;
                            break;
                        }
                    }
                    if (!opposed_to_observation) {
                        continue;
                    }
                    int higher_positive = 0;
                    int higher_similar = 0;
                    for (k = index + 1; k < applicable_rules.size; ++k) {
                        const unsigned int rule_to_compare_index = int_vector_get(&applicable_rules, k);
                        const Rule * const rule_to_compare = &(nerd->knowledge_base.active.
                        rules[rule_to_compare_index]);

                        int opposed_result =
                        literal_opposed(&(current_rule->head), &(rule_to_compare->head));
                        if (opposed_result > 0) {
                            higher_positive = opposed_result == 1;
                            higher_similar = opposed_result == 0;
                        }
                    }
                    if (!higher_positive) {
                        int_vector_push(&active_rules_to_demote, current_rule_index);
                    }
                }
            }


            for (index = 0; index < active_rules_to_demote.size; ++index) {
                int_vector_constructor(&demoted_applicable_rules);
                if (knowledge_base_demote_chained_rules(&(nerd->knowledge_base), &inferred, 
                &applicable_rules, ACTIVE, int_vector_get(&active_rules_to_demote, index),
                nerd->demotion_weight, &demoted_applicable_rules) == 1) {
                    ++rules_demoted;
                }

                rules_demoted += demoted_applicable_rules.size;

                for (k = index + 1; k < active_rules_to_demote.size; ++k) {
                    int active_rule_to_demote_subject_to_change =
                    int_vector_get(&active_rules_to_demote, k);
                    for (j = 0; j < demoted_applicable_rules.size; ++j) {
                        int active_deleted_rule = int_vector_get(&demoted_applicable_rules, j);

                        if (active_rule_to_demote_subject_to_change == active_deleted_rule) {
                            int_vector_delete(&active_rules_to_demote, k);
                            ++index;
                        } else if (active_rule_to_demote_subject_to_change > active_deleted_rule) {
                            --active_rules_to_demote.items[k];
                        }
                    }
                }

                int_vector_destructor(&demoted_applicable_rules);
            }

            for (index = 0; index < nerd->knowledge_base.inactive.length - rules_demoted; ++index) {
                const unsigned rule_index = index - rules_promoted - rules_deleted;
                const Rule * const current_rule = &(nerd->knowledge_base.inactive.rules[rule_index]);

                if (rule_applicable(current_rule, &observed_and_inferred)) {
                    int rule_concurs_result = rule_concurs(current_rule, &observation);
                    if (rule_concurs_result) {
                        if (knowledge_base_promote_rule(&(nerd->knowledge_base), INACTIVE, rule_index,
                        nerd->promotion_weight)) {
                            ++rules_promoted;
                        }
                    } else if (rule_concurs_result == 0) {
                        int opposed_to_observation = 0;
                        for (k = 0; k < uncovered.size; ++k) {
                            if (literal_opposed(&(current_rule->head),
                            &(uncovered.observations[k])) == 1) {
                                opposed_to_observation = 1;
                                break;
                            }
                        }
                        if (!opposed_to_observation) {
                            continue;
                        }

                        if (knowledge_base_demote_chained_rules(&(nerd->knowledge_base), &inferred,
                        &applicable_rules, INACTIVE, rule_index, nerd->demotion_weight, NULL) == 2) {
                            ++rules_deleted;
                        }
                        // if (knowledge_base_demote_rule(&(nerd->knowledge_base), INACTIVE, rule_index,
                        // nerd->demotion_weight) == 2) {
                        //     ++rules_deleted;
                        // }
                    }
                }
            }

            str = knowledge_base_to_string(&(nerd->knowledge_base));
            printf("%s\n", str);
            free(str);

            int_vector_destructor(&active_rules_to_demote);
            int_vector_destructor(&demoted_applicable_rules);
            int_vector_destructor(&applicable_rules);
            scene_destructor(&uncovered);

            scene_destructor(&observation);
            scene_destructor(&observed_and_inferred);
            scene_destructor(&inferred);
        }

        // float total_accuracy = 0, accuracy;

        // for (i = 0; i < total_observations; ++i) {
        //     sensor_get_next_scene(&(nerd->sensor), &observation, 0, NULL);
        //     evaluate_all_literals(nerd, &observation, &accuracy);
        //     total_accuracy += accuracy;
        //     scene_destructor(&observation);
        // }

        // printf("Epoch %d KnolwedgeBase accuracy: %f\n", epoch, total_accuracy / total_observations);
    }
    printf("Total time: %.f\n", difftime(time(NULL), start));
}
