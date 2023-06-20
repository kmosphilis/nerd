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
const char * const test_directory, const char * const constraints_file) {
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

    current_string_length = strlen(temp_directory);
    total_string_length += current_string_length + 1;

    (*settings)->temp_file = (char *) malloc((current_string_length + strlen(_temp) + 1)
    * sizeof(char));
    sprintf((*settings)->temp_file, "%s%s", temp_directory, _temp);

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
 * @brief Calls Prudens-JS using Node-JS. It create a file with a converted KnowledgeBase and a
 * Scene/Context, which holds an observation and saves the inferred Literals.
 *
 * @param knowledge_base The KnowledgeBase to be used in Prudens-JS.
 * @param observation A Scene/Context, which includes all the observed Literals.
 * @param inferred A Scene to save the inferred Literals by Prudens-JS. Deallocate using
 * scene_destructor.
*/
void prudensjs_inference(const PrudensSettings_ptr settings,
const KnowledgeBase * const knowledge_base, const Scene * const restrict observation,
Scene ** const inferred) {
    char *knowledge_base_prudensjs = knowledge_base_to_prudensjs(knowledge_base),
    *context_prudensjs = context_to_prudensjs(observation);
    if (!(knowledge_base_prudensjs && context_prudensjs)) {
        return;
    }

    FILE *file = fopen(settings->temp_file, "wb");

    fprintf(file, "%s\n%s", knowledge_base_prudensjs, context_prudensjs);
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
    *inferred = scene_constructor(true);
    Literal *literal;
    while (fscanf(file, "%s", buffer) != EOF) {
        literal = literal_constructor_from_string(buffer);
        scene_add_literal(*inferred, &literal);
    }

    fclose(file);
    remove(settings->temp_file);
}
