#include <stdio.h>
#include <stdint.h>

#include "scene.h"

#ifndef __SENSOR_H__
#define __SENSOR_H__

typedef struct Sensor {
    FILE *enviroment;
    uint_fast8_t reuse;
} Sensor;

Sensor *sensor_constructor_from_file(const char * const filepath, const uint_fast8_t reuse);
void sensor_destructor(Sensor ** const sensor);
void sensor_get_next_scene(const Sensor * const sensor, Scene ** const restrict output,
const unsigned short partial_observation, Scene ** const restrict initial_observation);

#endif