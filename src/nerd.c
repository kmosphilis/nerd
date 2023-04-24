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
 * @param filepath The path to the file containing the learning instances for the Nerd algorithm.
 * @param delimiter The delimiter which separates each observation in the given file.
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
Nerd *nerd_constructor(const char * const filepath, const char delimiter, const bool reuse,
const float activation_threshold, const unsigned int breadth, const unsigned int depth,
const unsigned int epochs, const float promotion_weight, const float demotion_weight,
const bool partial_observation) {
    if (filepath) {
        Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));
        if ((nerd->sensor = sensor_constructor_from_file(filepath, delimiter, reuse))) {
            nerd->knowledge_base = knowledge_base_constructor(activation_threshold);
            nerd->breadth = breadth;
            nerd->depth = depth;
            nerd->epochs = epochs;
            nerd->promotion_weight = promotion_weight;
            nerd->demotion_weight = demotion_weight;
            nerd->partial_observation = partial_observation;
            return nerd;
        }
        free(nerd);
    }
    return NULL;
}

/**
 * @brief Constructs a Nerd structure (object) using an existing nerd file.
 *
 * @param filepath The path to the file that contains previous a Nerd structure parameters (except
 * the number of epochs) and the learnt KnowledgeBase.
 * @param epochs The number of epochs the algorithm should learn for.
 *
 * @return A new Nerd object * from the given filepath. Use nerd_destructor to deallocate.
*/
Nerd *nerd_constructor_from_file(const char * const filepath, const unsigned int epochs) {
    if (filepath) {
        FILE *file = fopen(filepath, "rb");
        if (!file) {
            return NULL;
        }
        Nerd *nerd = (Nerd *) malloc(sizeof(Nerd));
        size_t buffer_size = BUFFER_SIZE;
        char *buffer = (char *) malloc(buffer_size * sizeof(char)), sensor_delimiter;
        unsigned short partial_observation, sensor_reuse;
        float activation_threshold;

        fscanf(file, "breadth: %zu\n", &(nerd->breadth));
        fscanf(file, "depth: %zu\n", &(nerd->depth));
        fscanf(file, "promotion_weight: %f\n", &(nerd->promotion_weight));
        fscanf(file, "demotion_weight: %f\n", &(nerd->demotion_weight));
        fscanf(file, "partial_observation: %hu\n", &partial_observation);
        nerd->partial_observation = partial_observation;
        fscanf(file, "sensor: %s '%c' %hu\n", buffer, &sensor_delimiter, &sensor_reuse);
        nerd->sensor = sensor_constructor_from_file(buffer, sensor_delimiter, sensor_reuse);
        free(buffer);

        fscanf(file, "knowledge_base:\n");
        fscanf(file, "\tactivation_threshold: %f\n", &activation_threshold);
        nerd->knowledge_base = knowledge_base_constructor(activation_threshold);

        fscanf(file, "\trules:\n");

        fpos_t position;
        fgetpos(file, &position);
        char *tokens;
        Literal *literal;
        Rule *rule;
        Body *body = scene_constructor(true);

        buffer = (char *) malloc(buffer_size * sizeof(char));
        memset(buffer, 0, buffer_size);


        while(fgets(buffer, buffer_size, file) != NULL) {
            if (strlen(buffer) == buffer_size) {
                buffer_size <<= 1;
                buffer = realloc(buffer, buffer_size * sizeof(char));
                fsetpos(file, &position);
                continue;
            }

            tokens = strtok(buffer, " (),\n");

            while (tokens != NULL) {
                if (strcmp(tokens, "=>") == 0) {
                    tokens = strtok(NULL, " (),\n");
                    literal = literal_constructor_from_string(tokens);

                    tokens = strtok(NULL, " (),\n");
                    float weight = atof(tokens);

                    rule = rule_constructor(body->size, body->literals, &literal, weight, true);
                    knowledge_base_add_rule(nerd->knowledge_base, &rule);

                    scene_destructor(&body);
                    body = scene_constructor(true);
                } else {
                    if (tokens[0] == '-') {
                        literal = literal_constructor(tokens + 1, 0);
                    } else {
                        literal = literal_constructor(tokens, 1);
                    }
                    scene_add_literal(body, &literal);
                }
                tokens = strtok(NULL, " (),\n");
            }
            fgetpos(file, &position);
        }

        scene_destructor(&body);

        free(buffer);

        fclose(file);
        nerd->epochs = epochs;
        return nerd;
    }
    return NULL;
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

/**
 * @brief Calls prudens-js using node-js. It create a file with a converted KnowledgeBase and a
 * Scene/Context, which holds an observation and saves the inferred Literals.
 *
 * @param knowledge_base The KnowledgeBase to be used in prudens-js.
 * @param observation A Scene/Context, which includes all the observed Literals.
 * @param inferred A Scene to save the inferred Literals by prudens-js. Deallocate using
 * scene_destructor.
*/
void prudensjs_inference(const KnowledgeBase * const knowledge_base,
const Scene * const restrict observation, Scene ** const inferred) {
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
    *inferred = scene_constructor(true);
    Literal *literal;
    while (fscanf(file, "%s", buffer) != EOF) {
        literal = literal_constructor_from_string(buffer);
        scene_add_literal(*inferred, &literal);
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

    Scene *observation = NULL, *inferred = NULL, *uncovered = NULL;
    unsigned int epoch, iteration;
    const size_t total_observations = sensor_get_total_observations(nerd->sensor);

    char *str = knowledge_base_to_string(nerd->knowledge_base);
    printf("%s\n", str);
    free(str);
    time_t start = time(NULL);

    for (epoch = 0; epoch < nerd->epochs; ++epoch) {
        for (iteration = 0; iteration < total_observations; ++iteration) {
            printf("\nEpoch %d of %zu, Iteration %d of %zu\n", epoch + 1, nerd->epochs,
            iteration + 1, total_observations);

            sensor_get_next_scene(nerd->sensor, &observation, nerd->partial_observation, NULL);
            prudensjs_inference(nerd->knowledge_base, observation, &inferred);

            knowledge_base_create_new_rules(nerd->knowledge_base, observation, inferred,
            nerd->breadth, 5);

            scene_difference(observation, inferred, &uncovered);

            str = scene_to_string(observation);
            printf("Observed: %s\n", str);
            free(str);
            str = scene_to_string(inferred);
            printf("Inferred: %s\n", str);
            free(str);
            str = scene_to_string(uncovered);
            printf("Observed ∖ Inferred: %s\n", str);
            free(str);

            rule_hypergraph_update_rules(nerd->knowledge_base, observation, inferred,
            nerd->promotion_weight, nerd->demotion_weight);

            str = knowledge_base_to_string(nerd->knowledge_base);
            printf("%s\n", str);
            free(str);

            scene_destructor(&uncovered);
            scene_destructor(&observation);
            scene_destructor(&inferred);
        }

        // float total_accuracy = 0, accuracy;

        // for (index = 0; index < total_observations; ++index) {
        //     sensor_get_next_scene(&(nerd->sensor), &observation, 0, NULL);
        //     evaluate_all_literals(nerd, &observation, &accuracy);
        //     total_accuracy += accuracy;
        //     scene_destructor(&observation);
        // }


        // printf("Epoch %d KnolwedgeBase accuracy: %f\n", epoch + 1,
        // total_accuracy / total_observations);
    }
    printf("Total time: %.f\n", difftime(time(NULL), start));
}

/**
 * @brief Saves/Converts the Nerd structure to a file which all the parameters that were used and the learnt
 * KnowledgeBase are saved, except the number of epochs.
 *
 * @param nerd The Nerd structure to be saved/converted to a file.
 * @param filepath The path and the name of the file which the Nerd structure will be saved to.
*/
void nerd_to_file(const Nerd * const nerd, const char * const filepath) {
    if (!(nerd && filepath)) {
        return;
    }

    FILE *file = fopen(filepath, "wb");
    unsigned int i;
    char *str = NULL;

    fprintf(file, "breadth: %zu\n", nerd->breadth);
    fprintf(file, "depth: %zu\n", nerd->depth);
    fprintf(file, "promotion_weight: %f\n", nerd->promotion_weight);
    fprintf(file, "demotion_weight: %f\n", nerd->demotion_weight);
    fprintf(file, "partial_observation: %hu\n", nerd->partial_observation);
    fprintf(file, "sensor: %s '%c' %hu\n", nerd->sensor->filepath, nerd->sensor->delimiter,
    nerd->sensor->reuse);

    fprintf(file, "knowledge_base:\n");
    fprintf(file, "  activation_threshold: %f\n", nerd->knowledge_base->activation_threshold);
    fprintf(file, "  rules:\n");

    for (i = 0; i < nerd->knowledge_base->active->length; ++i) {
        str = rule_to_string(nerd->knowledge_base->active->rules[i]);
        fprintf(file, "    %s,\n", str);
        free(str);
    }

    RuleQueue *inactive_rules;
    rule_hypergraph_get_inactive_rules(nerd->knowledge_base, &inactive_rules);

    for (i = 0; i < inactive_rules->length; ++i) {
        str = rule_to_string(inactive_rules->rules[i]);
        fprintf(file, "    %s,\n", str);
        free(str);
    }
    rule_queue_destructor(&inactive_rules);

    fclose(file);
}
