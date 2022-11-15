#include <stdio.h>

#include "scene.h"

#ifndef __SENSOR_H__
#define __SENSOR_H__

typedef struct Sensor {
    FILE *enviroment;
    unsigned short reuse;
} Sensor;

void sensor_constructor_from_file(Sensor * const sensor, const char * const filepath,
const unsigned short reuse);
void sensor_destructor(Sensor * const sensor);
void sensor_get_next_scene(const Sensor * const sensor, Scene * const restrict output,
const unsigned short partial_observation, Scene * const restrict initial_observation);

#endif