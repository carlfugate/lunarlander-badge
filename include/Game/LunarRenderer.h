#ifndef LUNAR_RENDERER_H
#define LUNAR_RENDERER_H

#ifndef NATIVE_TEST
#include <lvgl.h>
#endif

#include "LunarState.h"

// Full-world view: scale 1200x800 world to 320x240 screen (same as web/mobile)
#define LN_SCALE_X (LN_SCREEN_W / (float)LN_WORLD_W)
#define LN_SCALE_Y (LN_SCREEN_H / (float)LN_WORLD_H)

struct Camera {
    float x, y;
    float target_x, target_y;
    float lerp_speed;
};

#ifndef NATIVE_TEST
void renderer_init(lv_obj_t *parent);
void renderer_draw(const GameState &gs);
void renderer_cleanup();
#endif

void camera_update(Camera &cam, const Lander &l, int phase);
int16_t world_to_screen_x(float wx, const Camera &cam);
int16_t world_to_screen_y(float wy, const Camera &cam);

#endif
