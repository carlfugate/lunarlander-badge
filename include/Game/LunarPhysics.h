#ifndef LUNAR_PHYSICS_H
#define LUNAR_PHYSICS_H

#include "LunarConfig.h"

struct Lander {
    float x, y;
    float vx, vy;
    float rotation;
    float fuel;
    bool crashed;
    bool landed;
    bool thrusting;
};

void lander_init(Lander &l);
void lander_update(Lander &l, bool thrust, int rotate_dir, float dt);
float lander_speed(const Lander &l);

#endif
