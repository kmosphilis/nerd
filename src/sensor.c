#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sensor.h"

#define BUFFER_SIZE 255

/**
 * @brief Constructs a Sensor from a file.
 * 
 * @param sensor The Sensor to be constructed. If NULL is given, nothing will happen.
 * @param filepath The path to the file. If NULL is given, the sensor will not be constructed.
 * @param reuse Specifies whether to reuse the stream from the beginning when it reaches the EOF. 
 * Use > 0 to indicate yes, and <= 0 to indicate no.
 */
void sensor_construct_from_file(Sensor * const sensor, const char * const filepath,
const short reuse) {
    if (sensor != NULL) {
        if (filepath != NULL) {
            sensor->enviroment = fopen(filepath, "rb");
            if (sensor->enviroment == NULL) {
                sensor->reuse = -1;
            } else {
                sensor->reuse = reuse;
            }
        } else {
            sensor->enviroment = NULL;
            sensor->reuse = -1;
        }
    }
}

/**
 * @brief Destructs a Sensor.
 * 
 * @param sensor The Sensor to be destructed.
 */
void sensor_destruct(Sensor * const sensor) {
    if (sensor != NULL) {
        if (sensor->enviroment != NULL) {
            fclose(sensor->enviroment);
            sensor->enviroment = NULL;
            sensor->reuse = -1;
        }
    }
}

/**
 * @brief Gets the next Scene from a Sensor.
 * 
 * @param sensor The Sensor to extract the next Scene. If NULL, nothing will happen.
 * @param scene The Scene that will be extracted will be saved here. If NULL, nothing will happen.
 */
void sensor_get_next_scene(const Sensor * const sensor, Scene * const scene) {
    if ((sensor != NULL) && (scene != NULL)) {
        if (sensor->enviroment != NULL) {
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

            while ((c != EOF) && (c != '\n')) {
                if (!isspace((char) c)) {
                    buffer[i] = (char) c;
                    ++i;
                } else {
                    Literal literal;
                    if (buffer[0] == '-') {
                        literal_construct(&literal, buffer + 1, 0);
                    } else {
                        literal_construct(&literal, buffer, 1);
                    }
                    scene_add_literal(scene, &literal);
                    literal_destruct(&literal);
                    memset(buffer, 0, BUFFER_SIZE);
                    i = 0;
                }

                c = fgetc(sensor->enviroment);
            }

            Literal literal;
            if (buffer[0] == '-') {
                literal_construct(&literal, buffer + 1, 0);
            } else {
                literal_construct(&literal, buffer, 1);
            }
            scene_add_literal(scene, &literal);

            literal_destruct(&literal);
            free(buffer);
        }
    }
}