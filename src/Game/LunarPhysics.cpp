#include "Game/LunarPhysics.h"
#include <math.h>

void lander_init(Lander &l) {
    l.x = LN_START_X;
    l.y = LN_START_Y;
    l.vx = 0.0f;
    l.vy = 0.0f;
    l.rotation = 0.0f;
    l.fuel = LN_INITIAL_FUEL;
    l.crashed = false;
    l.landed = false;
    l.thrusting = false;
}

void lander_update(Lander &l, bool thrust, int rotate_dir, float dt) {
    if (l.crashed || l.landed) return;

    // Rotation
    l.rotation += rotate_dir * LN_ROTATION_SPEED * dt;
    if (l.rotation > M_PI) l.rotation -= 2.0f * M_PI;
    if (l.rotation < -M_PI) l.rotation += 2.0f * M_PI;

    // Thrust
    l.thrusting = thrust && l.fuel > 0.0f;
    if (l.thrusting) {
        l.vx += sinf(l.rotation) * LN_THRUST_POWER * dt;
        l.vy -= cosf(l.rotation) * LN_THRUST_POWER * dt;
        l.fuel -= LN_FUEL_CONSUMPTION * dt;
        if (l.fuel < 0.0f) l.fuel = 0.0f;
    }

    // Gravity
    l.vy += LN_GRAVITY * dt;

    // Position
    l.x += l.vx * dt;
    l.y += l.vy * dt;
}

float lander_speed(const Lander &l) {
    return sqrtf(l.vx * l.vx + l.vy * l.vy);
}
