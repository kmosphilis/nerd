#include <stdio.h>

#include "scene.h"

#ifndef __SENSOR_H__
#define __SENSOR_H__

#define BUFFER_SIZE 255

typedef struct Sensor {
    FILE *environment;
    unsigned short reuse;
} Sensor;

void sensor_constructor_from_file(Sensor * const sensor, const char * const filepath,
const unsigned short reuse);
void sensor_destructor(Sensor * const sensor);
size_t sensor_get_total_observations(const Sensor * const sensor);
void sensor_get_next_scene(const Sensor * const sensor, Scene * const restrict output,
const unsigned short partial_observation, Scene * const restrict initial_observation);

#endif