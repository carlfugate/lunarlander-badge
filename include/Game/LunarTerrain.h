#ifndef LUNAR_TERRAIN_H
#define LUNAR_TERRAIN_H

#include "LunarConfig.h"
#include "LunarPhysics.h"
#include <stdint.h>

struct Terrain {
    int16_t points[LN_MAX_TERRAIN_POINTS][2];
    uint16_t num_points;
    int16_t zone_x1, zone_x2, zone_y;
};

void terrain_generate(Terrain &t, uint8_t difficulty, uint32_t seed);
float terrain_height_at(const Terrain &t, float x);
bool terrain_is_landing_zone(const Terrain &t, float x);
bool terrain_check_collision(const Terrain &t, Lander &l);

#endif
