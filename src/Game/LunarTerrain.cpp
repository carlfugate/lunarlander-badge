#include "Game/LunarTerrain.h"
#include <stdlib.h>
#include <math.h>

// Difficulty settings: {step, variation, landing_width, y_base_offset, y_clamp_top_offset}
// y_base = WORLD_H - y_base_offset, clamp range: [WORLD_H - y_clamp_top_offset, WORLD_H - 50]
static const int16_t DIFF_STEP[]      = {50, 40, 30};
static const int16_t DIFF_VAR[]       = {20, 30, 50};
static const int16_t DIFF_ZONE_W[]    = {100, 80, 60};
static const int16_t DIFF_YBASE[]     = {100, 150, 200};
static const int16_t DIFF_CLAMP_TOP[] = {200, 300, 500};

static int16_t clamp(int16_t v, int16_t lo, int16_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int16_t rand_range(int16_t lo, int16_t hi) {
    return lo + (int16_t)(rand() % (hi - lo + 1));
}

void terrain_generate(Terrain &t, uint8_t difficulty, uint32_t seed) {
    srand(seed);
    if (difficulty > 2) difficulty = 2;

    int16_t step     = DIFF_STEP[difficulty];
    int16_t var      = DIFF_VAR[difficulty];
    int16_t zone_w   = DIFF_ZONE_W[difficulty];
    int16_t y_base   = LN_WORLD_H - DIFF_YBASE[difficulty];
    int16_t clamp_lo = LN_WORLD_H - DIFF_CLAMP_TOP[difficulty];
    int16_t clamp_hi = LN_WORLD_H - 50;

    // Landing zone position aligned to step
    int16_t zone_start = (int16_t)(rand_range(400 / step, 700 / step) * step);
    int16_t zone_end   = zone_start + zone_w;
    int16_t zone_y     = y_base;

    t.zone_x1 = zone_start;
    t.zone_x2 = zone_end;
    t.zone_y  = zone_y;

    int16_t y = y_base;
    t.num_points = 0;

    for (int16_t x = 0; x <= LN_WORLD_W; x += step) {
        if (t.num_points >= LN_MAX_TERRAIN_POINTS) break;

        if (x >= zone_start && x <= zone_end) {
            y = zone_y;
        } else {
            y += rand_range(-var, var);
            y = clamp(y, clamp_lo, clamp_hi);
            // Ensure non-landing points differ from zone height by >= 10
            if (abs(y - zone_y) < 10) {
                y = zone_y + ((rand() % 2) ? 15 : -15);
            }
        }

        t.points[t.num_points][0] = x;
        t.points[t.num_points][1] = y;
        t.num_points++;
    }
}

float terrain_height_at(const Terrain &t, float x) {
    for (uint16_t i = 0; i < t.num_points - 1; i++) {
        float x1 = t.points[i][0];
        float x2 = t.points[i + 1][0];
        if (x >= x1 && x <= x2) {
            float frac = (x - x1) / (x2 - x1);
            return t.points[i][1] + (t.points[i + 1][1] - t.points[i][1]) * frac;
        }
    }
    return LN_WORLD_H;
}

bool terrain_is_landing_zone(const Terrain &t, float x) {
    return x >= t.zone_x1 && x <= t.zone_x2;
}

bool terrain_check_collision(const Terrain &t, Lander &l) {
    // Out of bounds → crash
    if (l.x < 0 || l.x > LN_WORLD_W) {
        l.crashed = true;
        return true;
    }

    float ground = terrain_height_at(t, l.x);
    if (l.y < ground) return false;

    // Contact with terrain
    if (terrain_is_landing_zone(t, l.x) &&
        l.vy < LN_MAX_LANDING_SPEED &&
        fabsf(l.rotation) < LN_MAX_LANDING_ANGLE) {
        l.landed = true;
        l.y = ground - 10;  // offset so triangle base sits on terrain
        l.vx = 0;
        l.vy = 0;
    } else {
        l.crashed = true;
    }
    return true;
}
