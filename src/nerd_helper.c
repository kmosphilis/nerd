#include <string.h>

#include "nerd_utils.h"
#include "nerd_helper.h"

static const char *_node = "node";
static const char *_prudensjs_dir = "prudens-js/prudens-infer.js";
static const char *_temp = ".temp";

typedef struct PrudensSettings {
    char *temp_file;
    char *prudensjs_call;
} PrudensSettings;

int prudensjs_settings_constructor(PrudensSettings_ptr *settings, const char * const argv0,
const char * const test_directory, const char * const constraints_file, char *extra_args) {
    if (!settings) {
        return -1;
    }

    if (!argv0) {
        return -2;
    }
    int error_code = 0;

    const size_t current_directory_size = strstr(argv0, "bin") - argv0;
    char *current_directory = (char *) malloc((current_directory_size + 1) * sizeof(char));
    memset(current_directory, '\0', current_directory_size + 1);
    memcpy(current_directory, argv0, current_directory_size);

    size_t current_string_length = 0, string_set_so_far = 0,
    total_string_length = strlen(_node) + 1 + strlen(current_directory) + strlen(_prudensjs_dir) + 1
    + strlen(_temp) + 1;
    if (extra_args) {
        total_string_length += strlen(extra_args);
    }

    (*settings) = (PrudensSettings *) malloc(sizeof(PrudensSettings));
    (*settings)->prudensjs_call = (char *) malloc(total_string_length * sizeof(char));
    current_string_length = strlen(_node);
    memcpy((*settings)->prudensjs_call, _node, current_string_length);
    string_set_so_far += current_string_length;

    memcpy((*settings)->prudensjs_call + string_set_so_far, " ", 1);
    ++string_set_so_far;

    current_string_length = strlen(current_directory);
    memcpy((*settings)->prudensjs_call + string_set_so_far, current_directory,
    current_string_length);
    string_set_so_far += current_string_length;

    current_string_length = strlen(_prudensjs_dir);
    memcpy((*settings)->prudensjs_call + string_set_so_far, _prudensjs_dir, current_string_length);
    string_set_so_far += current_string_length;

    const char *temp_directory = test_directory;
    if ((!test_directory) ||
    (test_directory && (test_directory[strlen(test_directory) - 1] != '/'))) {
        error_code = 1;
        temp_directory = current_directory;
    }

    current_string_length = strlen(temp_directory) + strlen(_temp) + 1;
    if (extra_args) {
        current_string_length += strlen(extra_args);
    }
    total_string_length += current_string_length + 1;

    (*settings)->temp_file = (char *) malloc(current_string_length * sizeof(char));
    if (extra_args) {
        sprintf((*settings)->temp_file, "%s%s%s", temp_directory, _temp, extra_args);
    } else {
        sprintf((*settings)->temp_file, "%s%s", temp_directory, _temp);
    }

    (*settings)->prudensjs_call = (char *) realloc((*settings)->prudensjs_call,
    total_string_length * sizeof(char));

    memcpy((*settings)->prudensjs_call + string_set_so_far, " ", 1);
    ++string_set_so_far;

    current_string_length = strlen((*settings)->temp_file);
    memcpy((*settings)->prudensjs_call + string_set_so_far, (*settings)->temp_file,
    current_string_length);
    string_set_so_far += current_string_length;

    if (constraints_file) {
        FILE *file = fopen(constraints_file, "r");
        if (!file) {
            error_code = 2;
        } else {
            fclose(file);
            current_string_length = strlen(constraints_file);
            total_string_length += current_string_length + 1;

            (*settings)->prudensjs_call = (char *) realloc((*settings)->prudensjs_call,
            total_string_length * sizeof(char));

            memcpy((*settings)->prudensjs_call + string_set_so_far, " ", 1);
            ++string_set_so_far;

            memcpy((*settings)->prudensjs_call + string_set_so_far, constraints_file,
            current_string_length);
            string_set_so_far += current_string_length;
        }
    }

    memset((*settings)->prudensjs_call + string_set_so_far, '\0', 1);
    free(current_directory);
    return error_code;
}

int prudensjs_settings_destructor(PrudensSettings_ptr *settings) {
    if (!(settings && *settings)) {
        return 1;
    }

    safe_free((*settings)->prudensjs_call);
    safe_free((*settings)->temp_file);
    safe_free(*settings);

    return 0;
}

/**
 * @brief Calls Prudens-JS using Node-JS. It creates a file with a converted KnowledgeBase and a
 * Scene/Context, which holds an observation and saves the inference.
 *
 * @param knowledge_base The KnowledgeBase to be used in Prudens-JS.
 * @param observation A Scene/Context *, which includes all the observed Literals.
 * @param inference A Scene ** (reference to a Scene *) to save the inferences made by Prudens-JS.
 * Deallocate using scene_destructor.
*/
void prudensjs_inference(const PrudensSettings_ptr settings,
const KnowledgeBase * const knowledge_base, const Scene * const restrict observation,
Scene ** const inference) {
    if (!settings) {
        return;
    }

    char *knowledge_base_prudensjs = knowledge_base_to_prudensjs(knowledge_base),
    *context_prudensjs = context_to_prudensjs(observation);
    if (!(knowledge_base_prudensjs && context_prudensjs)) {
        return;
    }

    FILE *file = fopen(settings->temp_file, "wb");

    fprintf(file, "%s\n%s\n", knowledge_base_prudensjs, context_prudensjs);
    free(knowledge_base_prudensjs);
    free(context_prudensjs);
    fclose(file);
    system(settings->prudensjs_call);

    file = fopen(settings->temp_file, "rb");
    if (feof(file)) {
        fclose(file);
        return;
    }

    char buffer[BUFFER_SIZE];
    *inference = scene_constructor(true);
    Literal *literal;
    while (fscanf(file, "%s", buffer) != EOF) {
        literal = literal_constructor_from_string(buffer);
        scene_add_literal(*inference, &literal);
    }

    fclose(file);
    remove(settings->temp_file);
}

/**
 * @brief Calls Prudens-JS using Node-JS. It creates a file with a converted KnowledgeBase and a
 * number n of Scene/Context, which holds n observations and saves the inference for each one of
 * them.
 *
 * @param knowledge_base The KnowledgeBase to be used in Prudens-JS.
 * @param observations_size The number of different observations given.
 * @param observations A Scene/Context ** containing a number of different observations, where each
 * one includes their own observed Literals.
 * @param inferences A Scene *** (reference to a Scene **) to save the inferences for each
 * observation made by Prudens-JS. It has the same size as the observations (observations_size).
 * Deallocate each scene using scene_destructor.
 *
 * @return 1 if settings are NULL, 3 if observations_size is 0, 4 if observations is NULL, 5 if
 * the knowledge_base is NULL, -1 if the file opening failed, and 0 if it no errors occured.
*/
int prudensjs_inference_batch(const PrudensSettings_ptr settings,
const KnowledgeBase * const knowledge_base, const size_t observations_size,
Scene ** restrict observations, Scene *** const inferences) {
    if (!settings) {
        return 1;
    }

    if ((observations_size == 0)) {
        return 3;
    }

    if (!observations) {
        return 4;
    }

    (*inferences) = (Scene **) malloc(sizeof(Scene *) * observations_size);

    char *str = knowledge_base_to_prudensjs(knowledge_base);
    if (!str) {
        return 5;
    }

    FILE *file = fopen(settings->temp_file, "wb");
    fprintf(file, "%s\n", str);
    safe_free(str);

    size_t i;
    for (i = 0; i < observations_size; ++i) {
        str = context_to_prudensjs(observations[i]);
        fprintf(file, "%s\n", str);
        free(str);
    }
    fclose(file);

    system(settings->prudensjs_call);

    file = fopen(settings->temp_file, "rb");
    if (feof(file)) {
        fclose(file);
        return -1;
    }

    size_t buffer_size = BUFFER_SIZE;
    char *buffer = (char *) calloc(buffer_size, sizeof(char));
    size_t buffer_loc = 0;

    Literal *l;
    int c;
    for (i = 0; i < observations_size; ++i) {
        (*inferences)[i] = scene_constructor(true);

        while ((c = fgetc(file)) != EOF) {
            if ((c != ' ') && (c != '\n')) {
                buffer[buffer_loc++] = c;

                if (buffer_loc == buffer_size) {
                    buffer_size <<= 1;
                    buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
                    memset(buffer + (buffer_size >> 1), 0, buffer_size >> 1);
                }
            } else {
                l = literal_constructor_from_string(buffer);
                scene_add_literal((*inferences)[i], &l);
                memset(buffer, 0, strlen(buffer));
                buffer_loc = 0;
                if (c == '\n') {
                    break;
                }
            }
        }
    }

    free(buffer);
    fclose(file);
    remove(settings->temp_file);
    return 0;
}
