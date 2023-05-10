#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "scene.h"

#ifndef SENSOR_H
#define SENSOR_H

#define BUFFER_SIZE 256

typedef struct Sensor {
    FILE *environment;
    char *filepath, **header;
    char delimiter;
    bool reuse;
    size_t header_size;
} Sensor;

Sensor *sensor_constructor_from_file(const char * const filepath, const char delimiter,
const bool reuse, const bool header);
void sensor_destructor(Sensor ** const sensor);
size_t sensor_get_total_observations(const Sensor * const sensor);
void sensor_get_next_scene(const Sensor * const sensor, Scene ** const restrict output,
const bool partial_observation, Scene ** const restrict initial_observation);

#endif
