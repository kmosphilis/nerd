#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pcg_variants.h>

#include "nerd_utils.h"
#include "sensor.h"

/**
 * @brief Constructs a Sensor from a file.
 *
 * @param filepath The path to the file. If NULL is given, the sensor will not be constructed.
 * @param delimiter The delimiter which separates each observation in the given file.
 * @param reuse Indicates whether to reuse the stream from the beginning when it reaches the EOF.
 * Use true (> 0) to indicate yes, and false (0) to indicate no.
 * @param header Indicates whether the file contains a header or no. Use true (> 0) for yes, and
 * false (0) for no.
 *
 * @return A new Sensor *, or NULL if filepath is NULL or does not exist. Use sensor_destructor to
 * deallocate.
 */
Sensor *sensor_constructor_from_file(const char * const filepath, const char delimiter,
const bool reuse, const bool header) {
    if (filepath) {
        FILE *environment = fopen(filepath, "rb");
        if (environment != NULL) {
            Sensor *sensor = (Sensor *) malloc(sizeof(Sensor));
            sensor->environment = environment;
            sensor->filepath = strdup(filepath);
            sensor->delimiter = delimiter;
            sensor->reuse = reuse;
            sensor->header = NULL;
            sensor->header_size = 0;

            if (header) {
                size_t buffer_size = BUFFER_SIZE;
                char *buffer = (char *) malloc(buffer_size * sizeof(char));
                memset(buffer, 0, buffer_size);
                unsigned int i = 0;
                int c = fgetc(sensor->environment);
                bool end_of_line = false;

                while (!end_of_line) {
                    if (c != sensor->delimiter) {
                        if (i + 1 == buffer_size) {
                            buffer_size *= 2;
                            buffer = (char *) realloc(buffer, buffer_size);
                        }

                        buffer[i++] = (char) c;
                    } else {
save_header:
                        sensor->header = (char **) realloc(sensor->header,
                        ++sensor->header_size * sizeof(char *));
                        sensor->header[sensor->header_size - 1] = strdup(buffer);
                        memset(buffer, 0, strlen(buffer));
                        i = 0;

                        if(end_of_line) {
                            goto end_header_loop;
                        }
                    }
                    c = fgetc(sensor->environment);

                    if ((c == EOF) || (c == '\n')) {
                        end_of_line = true;
                        goto save_header;
                    }
end_header_loop:
                }

                free(buffer);
            }
            return sensor;
        }
    }
    return NULL;
}

/**
 * @brief Destructs a Sensor.
 *
 * @param sensor The Sensor to be destructed.
 */
void sensor_destructor(Sensor ** const sensor) {
    if (sensor && (*sensor)) {
        if ((*sensor)->environment) {
            fclose((*sensor)->environment);
            (*sensor)->environment = NULL;
            safe_free((*sensor)->filepath);
            (*sensor)->delimiter = '\0';
            (*sensor)->reuse = false;
            unsigned int i;
            for (i = 0 ; i < (*sensor)->header_size; ++i) {
                safe_free((*sensor)->header[i]);
            }
            safe_free((*sensor)->header);
            (*sensor)->header_size = 0;
        }
        safe_free(*sensor);
    }
}

/**
 * @brief Finds the total literals in the environment.
 *
 * @param sensor The sensor to get the total literals from.
 *
 * @return The number of total literals, or -1 if the sensor is NULL.
*/
size_t sensor_get_total_observations(const Sensor * const sensor) {
    if (sensor && sensor->environment) {
        fpos_t current_possition;
        fgetpos(sensor->environment, &current_possition);
        fseek(sensor->environment, 0, SEEK_SET);

        size_t total_observations = 0;
        char c;
        for (c = getc(sensor->environment); c != EOF; c = getc(sensor->environment)) {
            if (c == '\n') {
                ++total_observations;
            }
        }
        fsetpos(sensor->environment, &current_possition);

        if (sensor->header) {
            --total_observations;
        }
        return total_observations;
    }

    return -1;
}

/**
 * @brief Gets the next Scene from a Sensor.
 *
 * @param sensor The Sensor to extract the next Scene. If NULL, nothing will happen.
 * @param output The Scene that will be extracted will be saved here. If NULL, nothing will happen.
 * @param partial_observation If > 0 is given, the output will contain a subset of the initial
 * observation literals with a cardinality, |output| = (1, output.size). If 0 is given, the output
 * will be the initial observation.
 * @param initial_observation If partial_observation is > 0, the initial observation will be saved
 * here. If NULL is given, it will not be saved.
 */
void sensor_get_next_scene(const Sensor * const sensor, Scene ** const restrict output,
const bool partial_observation, Scene ** const restrict initial_observation) {
    if (sensor && output) {
        *output = scene_constructor(true);
        if (sensor->environment) {
            int c = fgetc(sensor->environment);

            if (c == EOF) {
                if (sensor->reuse) {
                    fseek(sensor->environment, 0, SEEK_SET);
                    if (sensor->header) {
                        while ((c = fgetc(sensor->environment)) != '\n');
                    }
                    c = fgetc(sensor->environment);
                } else {
                    return;
                }
            }

            size_t buffer_size = BUFFER_SIZE;
            char *buffer = (char *) malloc(buffer_size * sizeof(char));
            memset(buffer, 0, buffer_size);
            unsigned int i = 0, read_literals = 0;
            Literal *literal = NULL;
            bool end_of_line = false;

            while (!end_of_line) {
                if (c != sensor->delimiter) {
                    if (i + 1 == buffer_size) {
                        buffer_size *= 2;
                        buffer = (char *) realloc(buffer, buffer_size);
                    }

                    buffer[i++] = (char) c;
                } else {
save_literal:
                    if (sensor->header) {
                        size_t header_size = strlen(sensor->header[read_literals]);
                        if (i + header_size + 1 >= buffer_size) {
                            buffer_size *= 2;
                            buffer = (char *) realloc(buffer, buffer_size);
                        }

                        memcpy((buffer + header_size + 1), buffer, strlen(buffer));
                        memcpy(buffer, sensor->header[read_literals], header_size);
                        memcpy(buffer + header_size, "_", 1);
                        ++read_literals;
                    }

                    literal = literal_constructor_from_string(buffer);
                    scene_add_literal(*output, &literal);
                    memset(buffer, 0, buffer_size);
                    i = 0;

                    if (end_of_line) {
                        goto end_literal_loop;
                    }
                }

                c = fgetc(sensor->environment);

                if ((c == EOF) || (c == '\n')) {
                    end_of_line = true;
                    goto save_literal;
                }
end_literal_loop:
            }

            free(buffer);

            if (partial_observation) {
                if (initial_observation){
                    scene_copy(initial_observation, *output);
                }

                pcg32_random_t seed;
                if (global_seed) {
                    seed = *global_seed;
                } else {
                    pcg32_srandom_r(&seed, time(NULL), 0U);
                }

                size_t number_of_literals = (*output)->size -
                (pcg32_random_r(&seed) % (*output)->size);

                if (number_of_literals == (*output)->size) {
                    return;
                } else if (number_of_literals == 1) {
                    scene_remove_literal(*output, pcg32_random_r(&seed) % (*output)->size, NULL);
                    return;
                }

                unsigned int *random_literals = (unsigned int *) calloc(number_of_literals,
                sizeof(int));
                unsigned int i;
                for (i = 0; i < number_of_literals; ++i) {
                    random_literals[i] = i;
                }

                for (i = 0; i < number_of_literals; ++i) {
                    int index = pcg32_random_r(&seed) % number_of_literals--;
                    scene_remove_literal(*output, random_literals[index], NULL);
                    random_literals[index] = random_literals[number_of_literals];
                }
                free(random_literals);
            }
        }
    }
}
