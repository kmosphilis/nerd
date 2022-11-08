#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "nerd.h"

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
 */
void nerd_constructor(Nerd * restrict nerd, const char * restrict filepath, unsigned short reuse,
const float activation_threshold, const unsigned int breadth, const unsigned int depth,
const unsigned int epochs, const float promotion_weight, const float demotion_weight) {
    if ((nerd != NULL) && (filepath != NULL)) {
        sensor_constructor_from_file(&(nerd->sensor), filepath, reuse);
        nerd->breadth = breadth;
        nerd->depth = depth;
        nerd->epochs = epochs;
        nerd->promotion_weight = promotion_weight;
        nerd->demotion_weight = demotion_weight;
        knowledge_base_constructor(&(nerd->knowledge_base), activation_threshold);
    }
}

/**
 * @brief Destructs a Nerd structure (object).
 * 
 * @param nerd The Nerd structure to be destructed.
 */
void nerd_destructor(Nerd * restrict nerd) {
    nerd->breadth = 0;
    nerd->depth = 0;
    nerd->epochs = 0;
    nerd->promotion_weight = INFINITY;
    nerd->demotion_weight = INFINITY;
    sensor_destructor(&(nerd->sensor));
    knowledge_base_destructor(&(nerd->knowledge_base));
}

void prudensjs_inference(const KnowledgeBase * restrict knowledge_base,
const Scene * restrict context, Scene * restrict result, Scene * restrict inferred_literals,
unsigned int ** inferred_literals_rule_numbers, int *** inferred_literals_rule_indices) {
    const char * const temp_filename = ".temp";
    char *knowledge_base_prudensjs = knowledge_base_to_prudensjs(knowledge_base),
    *context_prudensjs = context_to_prudensjs(context);
    if ((knowledge_base_prudensjs != NULL) && (context_prudensjs != NULL)) {
        FILE *file = fopen(temp_filename, "wb");

        fprintf(file, "%s\n%s", knowledge_base_prudensjs, context_prudensjs);
        fclose(file);
        system("node prudens-infer.js");

        file = fopen(temp_filename, "rb");
        if (feof(file)) {
            return;
        }

        char buffer[50];
        fpos_t file_pos;
        fgetpos(file, &file_pos);
        while (fgetc(file) != '\n') {
            fsetpos(file, &file_pos);
            fscanf(file, "%s", buffer);
            Literal l;
            if (buffer[0] == '-') {
                literal_constructor(&l, buffer + 1, 0);
            } else {
                literal_constructor(&l, buffer, 1);
            }
            scene_add_literal(result, &l);
            literal_destructor(&l);

            fgetpos(file, &file_pos);
        }

        unsigned int *inferred_heads_number_of_rules = NULL;

        while(fgetc(file) != '\n') {
            fsetpos(file, &file_pos);
            fscanf(file, " %[^, ],", buffer);
            Literal l;
            if (buffer[0] == '-') {
                literal_constructor(&l, buffer + 1, 0);
            } else {
                literal_constructor(&l, buffer, 1);
            }
            scene_add_literal(inferred_literals, &l);
            literal_destructor(&l);
            inferred_heads_number_of_rules = (unsigned int *) 
            realloc(inferred_heads_number_of_rules, (inferred_literals->size)
            * sizeof(unsigned int));

            fscanf(file, "%d", &(inferred_heads_number_of_rules[inferred_literals->size - 1]));

            fgetpos(file, &file_pos);
        }

        *inferred_literals_rule_numbers = inferred_heads_number_of_rules;

        int **inferred_heads_rules_indices = (int **) malloc(inferred_literals->size 
        * sizeof(int *));

        unsigned int i, j;
        for (i = 0; i < inferred_literals->size; ++i) {
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
void nerd_start_learning(Nerd * restrict nerd) {
    /*  1 - Create rules with heads that do not exist in effective rules (rules that were applicable and were correct).
        2 - Promote rules applicable on inferred that concur with observed. Promote both active and inactive rules.
            If head is the same as observed, promote, else demote it if it opposed.
        3 - Weak priority (it is implemented by default).
        4 - Demote applicable rules that oppose observed literals.
        5 - No priority for newly inactive rules (implemented by default).
    */

    Scene observation, inferred, inferred_literals;
    IntVector active_concurring_rules_observed, inactive_concurring_rules_observed,
    active_applicable_rules_inferred, inactive_applicable_rules_inferred;

    unsigned int *inferred_literals_rule_numbers = NULL;
    int **inferred_literals_rule_indices = NULL;
    unsigned int epoch, i, j, k;

    scene_constructor(&observation);
    scene_constructor(&inferred);
    scene_constructor(&inferred_literals);
    int_vector_constructor(&active_concurring_rules_observed);
    int_vector_constructor(&inactive_concurring_rules_observed);
    int_vector_constructor(&active_applicable_rules_inferred);
    int_vector_constructor(&inactive_applicable_rules_inferred);

    char *str = knowledge_base_to_string(&(nerd->knowledge_base));
    printf("%s\n", str);
    free(str);

    for (epoch = 0; epoch < nerd->epochs; ++epoch) {
        printf("Epoch %d of %d\n", epoch + 1, nerd->epochs);

        sensor_get_next_scene(&(nerd->sensor), &observation);

        prudensjs_inference(&(nerd->knowledge_base), &observation, &inferred, &inferred_literals,
        &inferred_literals_rule_numbers, &inferred_literals_rule_indices);

        knowledge_base_create_new_rules(&(nerd->knowledge_base), &observation, &inferred_literals,
        nerd->breadth, 5);

        knowledge_base_applicable_rules(&(nerd->knowledge_base), &inferred,
        &active_applicable_rules_inferred, &inactive_applicable_rules_inferred);
        knowledge_base_concurring_rules(&(nerd->knowledge_base), &observation,
        &active_concurring_rules_observed, &inactive_concurring_rules_observed);

        for (i = 0; i < active_applicable_rules_inferred.size; ++i) {
            int active_applicable_inferred_rule_index = active_applicable_rules_inferred.items[i];
            for (j = 0; j < active_concurring_rules_observed.size; ++j) {
                int active_concurring_rule_index = active_concurring_rules_observed.items[j];
                if (active_applicable_inferred_rule_index == 
                active_concurring_rule_index) {
                    knowledge_base_promote_rule(&(nerd->knowledge_base), ACTIVE,
                    active_applicable_inferred_rule_index, nerd->promotion_weight);
                    break;
                } else {
                    const Rule * const active_applicable_rule = &(nerd->knowledge_base.active.
                    rules[active_applicable_inferred_rule_index]);
                    const Rule * const active_concurring_rule = &(nerd->knowledge_base.active.
                    rules[active_concurring_rule_index]);
                    if (literal_opposed(&(active_applicable_rule->head),
                    &(active_concurring_rule->head)) == 1) {
                        if (active_concurring_rule_index > active_applicable_inferred_rule_index) {
                            unsigned int old_active_size = nerd->knowledge_base.active.length;
                            knowledge_base_demote_rule(&(nerd->knowledge_base), ACTIVE,
                            active_applicable_inferred_rule_index, nerd->demotion_weight);
                            if (old_active_size != nerd->knowledge_base.active.length) {
                                for (k = i; k < active_applicable_rules_inferred.size; ++k) {
                                    --active_applicable_rules_inferred.items[k];
                                }
                                unsigned int observed_rules_size =
                                active_concurring_rules_observed.size;
                                for (k = 0; k < observed_rules_size; ++k) {
                                    --active_concurring_rules_observed.items[k];
                                    if (active_concurring_rules_observed.items[k] < 0) {
                                        int_vector_delete(&active_concurring_rules_observed, k);
                                        --observed_rules_size;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        for (i = 0; i < inactive_applicable_rules_inferred.size; ++i) {
            int inactive_applicable_inferred_rule_index =
            inactive_applicable_rules_inferred.items[i];
            for (j = 0; j < inactive_concurring_rules_observed.size; ++j) {
                int inactive_concurring_inferred_rule_index =
                inactive_concurring_rules_observed.items[j];
                if (inactive_applicable_inferred_rule_index ==
                inactive_concurring_inferred_rule_index) {
                    unsigned int old_inactive_size = nerd->knowledge_base.inactive.length;
                    knowledge_base_promote_rule(&(nerd->knowledge_base), INACTIVE,
                    inactive_concurring_inferred_rule_index, nerd->promotion_weight);
                    if (old_inactive_size != nerd->knowledge_base.inactive.length) {
                        for (k = i; k < inactive_applicable_rules_inferred.size; ++k) {
                            --inactive_applicable_rules_inferred.items[k];
                        }
                        unsigned int observed_rules_size = inactive_concurring_rules_observed.size;
                        for (k = 0; k < observed_rules_size; ++k) {
                            --inactive_concurring_rules_observed.items[k];
                            if (inactive_concurring_rules_observed.items[k] < 0) {
                                int_vector_delete(&inactive_concurring_rules_observed, k);
                                --observed_rules_size;
                            }
                        }
                    }
                    break;
                } else {
                    const Rule * const inactive_applicable_rule = &(nerd->knowledge_base.inactive.
                    rules[inactive_applicable_inferred_rule_index]);
                    const Rule * const inactive_concurring_rule = &(nerd->knowledge_base.inactive.
                    rules[inactive_concurring_inferred_rule_index]);
                    if (literal_opposed(&(inactive_applicable_rule->head),
                    &(inactive_concurring_rule->head)) == 1) {
                        knowledge_base_demote_rule(&(nerd->knowledge_base), INACTIVE,
                        inactive_applicable_inferred_rule_index, nerd->demotion_weight);
                        break;
                    }
                }
            }
        }

        free(inferred_literals_rule_numbers);
        inferred_literals_rule_numbers = NULL;

        for (i = 0; i < inferred_literals.size; ++i) {
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
    }
    str = knowledge_base_to_string(&(nerd->knowledge_base));
    printf("%s\n", str);
    free(str);
}
