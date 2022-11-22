#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "nerd.h"

/**
 * @brief Constructs a Nerd structure (object).
 *
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
 *
 * @return A new Nerd object *. Use nerd_destructor to deallocate.
 */
Nerd *nerd_constructor(const char * const filepath, const uint_fast8_t reuse,
const float activation_threshold, const unsigned int breadth, const unsigned int depth,
const unsigned int epochs, const float promotion_weight, const float demotion_weight,
const uint_fast8_t partial_observation) {
    Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));
    if (filepath) {
        nerd->sensor = sensor_constructor_from_file(filepath, reuse);
        nerd->breadth = breadth;
        nerd->depth = depth;
        nerd->epochs = epochs;
        nerd->promotion_weight = promotion_weight;
        nerd->demotion_weight = demotion_weight;
        nerd->knowledge_base = knowledge_base_constructor(activation_threshold);
        nerd->partial_observation = partial_observation;
    } else {
        nerd->sensor = NULL;
        nerd->knowledge_base = NULL;
    }
    return nerd;
}

/**
 * @brief Destructs a Nerd structure (object).
 *
 * @param nerd The Nerd structure to be destructed. It should be a reference to the object's
 * pointer.
 */
void nerd_destructor(Nerd ** const nerd) {
    if (nerd && (*nerd)) {
        (*nerd)->breadth = 0;
        (*nerd)->depth = 0;
        (*nerd)->epochs = 0;
        (*nerd)->promotion_weight = INFINITY;
        (*nerd)->demotion_weight = INFINITY;
        sensor_destructor(&((*nerd)->sensor));
        knowledge_base_destructor(&((*nerd)->knowledge_base));
        (*nerd)->partial_observation = 0;
        free(*nerd);
        *nerd = NULL;
    }
}

void prudensjs_inference(const KnowledgeBase * const knowledge_base,
const Scene * const restrict context, Scene ** restrict result, Scene ** restrict inferred_literals,
unsigned int ** inferred_literals_rule_numbers, int *** inferred_literals_rule_indices) {
    const char * const temp_filename = ".temp";
    char *knowledge_base_prudensjs = knowledge_base_to_prudensjs(knowledge_base),
    *context_prudensjs = context_to_prudensjs(context);
    if (knowledge_base_prudensjs && context_prudensjs) {
        FILE *file = fopen(temp_filename, "wb");

        fprintf(file, "%s\n%s", knowledge_base_prudensjs, context_prudensjs);
        fclose(file);
        system("node prudens-infer.js");

        file = fopen(temp_filename, "rb");
        if (feof(file)) {
            return;
        }

        *result = scene_constructor();
        *inferred_literals = scene_constructor();
        Literal *l = NULL;
        char buffer[50];
        fpos_t file_pos;
        fgetpos(file, &file_pos);
        while (fgetc(file) != '\n') {
            fsetpos(file, &file_pos);
            fscanf(file, "%s", buffer);
            if (buffer[0] == '-') {
                l = literal_constructor(buffer + 1, 0);
            } else {
                l = literal_constructor(buffer, 1);
            }
            scene_add_literal(*result, l);
            literal_destructor(&l);

            fgetpos(file, &file_pos);
        }

        unsigned int *inferred_heads_number_of_rules = NULL;

        while(fgetc(file) != '\n') {
            fsetpos(file, &file_pos);
            fscanf(file, " %[^, ],", buffer);
            if (buffer[0] == '-') {
                l = literal_constructor(buffer + 1, 0);
            } else {
                l = literal_constructor(buffer, 1);
            }
            scene_add_literal(*inferred_literals, l);
            literal_destructor(&l);
            inferred_heads_number_of_rules = (unsigned int *) 
            realloc(inferred_heads_number_of_rules, ((*inferred_literals)->size)
            * sizeof(unsigned int));

            fscanf(file, "%d", &(inferred_heads_number_of_rules[(*inferred_literals)->size - 1]));

            fgetpos(file, &file_pos);
        }

        *inferred_literals_rule_numbers = inferred_heads_number_of_rules;

        int **inferred_heads_rules_indices = (int **) malloc((*inferred_literals)->size
        * sizeof(int *));

        unsigned int i, j;
        for (i = 0; i < (*inferred_literals)->size; ++i) {
            inferred_heads_rules_indices[i] = (int *) 
            malloc(inferred_heads_number_of_rules[i] * sizeof(int));
            for (j = 0; j < inferred_heads_number_of_rules[i]; ++j) {
                fscanf(file, "%d", &(inferred_heads_rules_indices[i][j]));
            }
        }
    
        *inferred_literals_rule_indices = inferred_heads_rules_indices;

        fclose(file);
        remove(temp_filename);
    }

    free(knowledge_base_prudensjs);
    free(context_prudensjs);
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

    Scene *observation = NULL, *inferred = NULL, *inferred_literals = NULL;
    IntVector *active_concurring_rules_observed = NULL, *inactive_concurring_rules_observed = NULL,
    *active_applicable_rules_inferred = NULL, *inactive_applicable_rules_inferred = NULL;

    unsigned int *inferred_literals_rule_numbers = NULL;
    int **inferred_literals_rule_indices = NULL;
    unsigned int epoch, index, i, k;

    char *str = knowledge_base_to_string(nerd->knowledge_base);
    printf("%s\n", str);
    free(str);

    for (epoch = 0; epoch < nerd->epochs; ++epoch) {
        printf("Epoch %d of %d\n", epoch + 1, nerd->epochs);

        sensor_get_next_scene(nerd->sensor, &observation, nerd->partial_observation, NULL);

        prudensjs_inference(nerd->knowledge_base, observation, &inferred, &inferred_literals,
        &inferred_literals_rule_numbers, &inferred_literals_rule_indices);

        knowledge_base_create_new_rules(nerd->knowledge_base, observation, inferred_literals,
        nerd->breadth, 5);
        Scene *uncovered = NULL;
        scene_difference(observation, inferred_literals, &uncovered);

        str = scene_to_string(observation);
        printf("Observed: %s\n", str);
        free(str);
        str = scene_to_string(inferred_literals);
        printf("Inferred: %s\n", str);
        free(str);
        str = scene_to_string(uncovered);
        printf("Observed ∖ Inferred: %s\n", str);
        free(str);

        unsigned int rules_demoted = 0, rules_promoted = 0;
        IntVector *rules_demoted_vector = NULL, *applicable_rules = NULL;
        rules_demoted_vector = int_vector_constructor();
        applicable_rules = int_vector_constructor();

        for (index = nerd->knowledge_base->active->length; index > 0; --index) {
            const Rule * const current_rule = nerd->knowledge_base->active->rules[index - 1];
            
            if (rule_applicable(current_rule, inferred)) {
                int_vector_push(applicable_rules, index - 1);
            }
        }

        printf("Applicable rule indices: ");
        for (index = 0; index < applicable_rules->size; ++index) {
            printf("%d ", applicable_rules->items[index]);
        }
        printf("\n");


        for (index = 0; index < applicable_rules->size; ++index) {
            const unsigned int current_rule_index = int_vector_get(applicable_rules, index);
            const Rule * const current_rule = nerd->knowledge_base->active
            ->rules[current_rule_index];

            int rule_concurs_result = rule_concurs(current_rule, observation);
            if (rule_concurs_result == 1) {
                  knowledge_base_promote_rule(nerd->knowledge_base, ACTIVE, current_rule_index,
                  nerd->promotion_weight);
            } else if (rule_concurs_result == 0) {
                int opposed_to_observation = 0;
                for (k = 0 ; k < uncovered->size; ++k) {
                    if (literal_opposed(current_rule->head, uncovered->observations[k]) == 1) {
                        opposed_to_observation = 1;
                        break;
                    }
                }
                if (!opposed_to_observation) {
                    continue;
                }
                int higher_positive = 0;
                int higher_similar = 0;
                for (k = index + 1; k < applicable_rules->size; ++k) {
                    const unsigned int rule_to_compare_index = int_vector_get(applicable_rules, k);
                    const Rule * const rule_to_compare = nerd->knowledge_base->active
                    ->rules[rule_to_compare_index];

                    int opposed_result = literal_opposed(current_rule->head, rule_to_compare->head);
                    if (opposed_result >= 0) {
                        higher_positive = opposed_result == 1;
                        higher_similar = opposed_result == 0;
                    }
                }
                if (!higher_positive) {
                    str = rule_to_string(current_rule);
                    if (knowledge_base_demote_rule(nerd->knowledge_base, ACTIVE, current_rule_index,
                     nerd->demotion_weight)) {
                        ++rules_demoted;
                        int_vector_delete(applicable_rules, index);
                    }
                    printf("Active Rule demoted: %s\n", str);
                    free(str);
                }
            }
        }

        for (index = 0; index < nerd->knowledge_base->inactive->length - rules_demoted; ++index) {
            const unsigned rule_index = index - rules_promoted;
            const Rule * const current_rule = nerd->knowledge_base->inactive->rules[rule_index];

            if (rule_applicable(current_rule, inferred)) {
                int rule_concurs_result = rule_concurs(current_rule, observation);
                if (rule_concurs_result) {
                    if (knowledge_base_promote_rule(nerd->knowledge_base, INACTIVE, rule_index,
                    nerd->promotion_weight)) {
                        ++rules_promoted;
                    }
                } else if (rule_concurs_result == 0) {
                    int opposed_to_observation = 0;
                    for (k = 0 ; k < uncovered->size; ++k) {
                        if (literal_opposed(current_rule->head, uncovered->observations[k]) == 1) {
                            opposed_to_observation = 1;
                            break;
                        }
                    }
                    if (!opposed_to_observation) {
                        continue;
                    }
                    knowledge_base_demote_rule(nerd->knowledge_base, INACTIVE, rule_index,
                    nerd->demotion_weight);
                    str = rule_to_string(current_rule);
                    printf("Inactive Rule demoted: %s\n", str);
                    free(str);
                }
            }
        }

        int_vector_destructor(&rules_demoted_vector);
        int_vector_destructor(&applicable_rules);
        scene_destructor(&uncovered);

        free(inferred_literals_rule_numbers);
        inferred_literals_rule_numbers = NULL;

        for (i = 0; i < inferred_literals->size; ++i) {
            free(inferred_literals_rule_indices[i]);
        }
        free(inferred_literals_rule_indices);
        inferred_literals_rule_indices = NULL;

        scene_destructor(&observation);
        scene_destructor(&inferred);
        scene_destructor(&inferred_literals);
        int_vector_destructor(&active_concurring_rules_observed);
        int_vector_destructor(&inactive_concurring_rules_observed);
        int_vector_destructor(&active_applicable_rules_inferred);
        int_vector_destructor(&inactive_applicable_rules_inferred);
        str = knowledge_base_to_string(nerd->knowledge_base);
        printf("%s\n", str);
        free(str);
    }
}
