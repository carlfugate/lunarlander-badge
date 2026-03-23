#ifndef LUNAR_RENDERER_H
#define LUNAR_RENDERER_H

#ifndef NATIVE_TEST
#include <lvgl.h>
#endif

#include "LunarState.h"

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
