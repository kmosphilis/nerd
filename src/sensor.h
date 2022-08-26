#include <stdio.h>

#include "scene.h"

#ifndef __SENSOR_H__
#define __SENSOR_H__

typedef struct Sensor {
    FILE *enviroment;
    short reuse;
} Sensor;

void sensor_construct_from_file(Sensor * const sensor, const char * const filepath,
const short reuse);
void sensor_get_next_scene(const Sensor * const sensor, Scene * const scene_to_save);
void sensor_destruct(Sensor * const sensor);

#endif