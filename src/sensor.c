#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "sensor.h"

#define BUFFER_SIZE 255

/**
 * @brief Constructs a Sensor from a file.
 *
 * @param filepath The path to the file. If NULL is given, the sensor will not be constructed.
 * @param reuse Specifies whether to reuse the stream from the beginning when it reaches the EOF. 
 * Use > 0 to indicate yes, and 0 to indicate no.
 *
 * @return A new Sensor object *. Use sensor_destructor to deallocate.
 */
Sensor *sensor_constructor_from_file(const char * const filepath, const uint_fast8_t reuse) {
    Sensor *sensor = (Sensor *) malloc(sizeof(Sensor));
    if (filepath) {
        sensor->enviroment = fopen(filepath, "rb");
        if (sensor->enviroment == NULL) {
            sensor->reuse = 0;
        } else {
            sensor->reuse = reuse;
        }
    } else {
        sensor->enviroment = NULL;
        sensor->reuse = 0;
    }
    return sensor;
}

/**
 * @brief Destructs a Sensor.
 *
 * @param sensor The Sensor to be destructed. It should be a reference to the object's pointer.
 */
void sensor_destructor(Sensor ** const sensor) {
    if (sensor && (*sensor)) {
        if ((*sensor)->enviroment) {
            fclose((*sensor)->enviroment);
            (*sensor)->enviroment = NULL;
            (*sensor)->reuse = 0;
        }
        free(*sensor);
        *sensor = NULL;
    }
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
const unsigned short partial_observation, Scene ** const restrict initial_observation) {
    if (sensor && output) {
        *output = scene_constructor();
        if (sensor->enviroment) {
            int c = fgetc(sensor->enviroment);

            if (c == EOF) {
                if (sensor->reuse) {
                    fseek(sensor->enviroment, 0, SEEK_SET);
                    c = fgetc(sensor->enviroment);
                } else {
                    return;
                }
            }

            char *buffer = (char *) malloc(BUFFER_SIZE * sizeof(char));
            memset(buffer, 0, BUFFER_SIZE);
            unsigned int i = 0;
            Literal *literal = NULL;

            while ((c != EOF) && (c != '\n')) {
                if (!isspace((char) c)) {
                    buffer[i] = (char) c;
                    ++i;
                } else {
                    if (buffer[0] == '-') {
                        literal = literal_constructor(buffer + 1, 0);
                    } else {
                        literal = literal_constructor(buffer, 1);
                    }
                    scene_add_literal(*output, literal);
                    literal_destructor(&literal);
                    memset(buffer, 0, BUFFER_SIZE);
                    i = 0;
                }

                c = fgetc(sensor->enviroment);
            }

            if (buffer[0] == '-') {
                literal = literal_constructor(buffer + 1, 0);
            } else {
                literal = literal_constructor(buffer, 1);
            }
            scene_add_literal(*output, literal);

            literal_destructor(&literal);
            free(buffer);

            if (partial_observation) {
                if (initial_observation){
                    scene_copy(initial_observation, *output);
                }
                srand(time(NULL));
                size_t number_of_literals = (*output)->size - (rand() % (*output)->size);

                if (number_of_literals == (*output)->size) {
                    return;
                } else if (number_of_literals == 1) {
                    scene_remove_literal(*output, rand() % (*output)->size);
                    return;
                }

                unsigned int *random_literals = (unsigned int *) calloc(number_of_literals,
                sizeof(int));
                unsigned int i;
                for (i = 0; i < number_of_literals; ++i) {
                    random_literals[i] = i;
                }

                for (i = 0; i < number_of_literals; ++i) {
                    int index = rand() % number_of_literals;
                    --number_of_literals;
                    scene_remove_literal(*output, random_literals[index]);
                    random_literals[index] = random_literals[number_of_literals];
                }
                free(random_literals);
            }
        }
    }
}