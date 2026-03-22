#include "Game/LunarRenderer.h"
#include "Game/LunarConfig.h"
#include <math.h>

// Camera functions — always compiled (used by tests)
void camera_update(Camera &cam, const Lander &l) {
    cam.x = l.x;
    cam.y = l.y;
    float half_w = LN_SCREEN_W / 2.0f;
    float half_h = LN_SCREEN_H / 2.0f;
    if (cam.x < half_w) cam.x = half_w;
    if (cam.x > LN_WORLD_W - half_w) cam.x = LN_WORLD_W - half_w;
    if (cam.y < half_h) cam.y = half_h;
    if (cam.y > LN_WORLD_H - half_h) cam.y = LN_WORLD_H - half_h;
}

int16_t world_to_screen_x(float wx, const Camera &cam) {
    return (int16_t)(wx - (cam.x - LN_SCREEN_W / 2));
}

int16_t world_to_screen_y(float wy, const Camera &cam) {
    return (int16_t)(wy - (cam.y - LN_SCREEN_H / 2));
}

#ifndef NATIVE_TEST

#include <esp_heap_caps.h>

#define NUM_STARS 40
#define LANDER_SIZE 8

static lv_obj_t *canvas = NULL;
static uint8_t *canvas_buf = NULL;
static Camera cam;

// HUD elements
static lv_obj_t *lbl_fuel = NULL;
static lv_obj_t *lbl_speed = NULL;
static lv_obj_t *lbl_alt = NULL;
static lv_obj_t *lbl_warn = NULL;
static lv_obj_t *bar_fuel = NULL;

// Pre-generated star positions (world coords)
static int16_t stars[NUM_STARS][2];
static bool stars_generated = false;

static void generate_stars() {
    if (stars_generated) return;
    uint32_t seed = 12345;
    for (int i = 0; i < NUM_STARS; i++) {
        seed = seed * 1103515245 + 12345;
        stars[i][0] = (int16_t)(seed % LN_WORLD_W);
        seed = seed * 1103515245 + 12345;
        stars[i][1] = (int16_t)(seed % (LN_WORLD_H / 2));
    }
    stars_generated = true;
}

static void draw_line(lv_obj_t *c, int16_t x1, int16_t y1, int16_t x2, int16_t y2, lv_color_t color) {
    lv_layer_t layer;
    lv_canvas_init_layer(c, &layer);
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = color;
    dsc.width = 1;
    dsc.p1.x = x1; dsc.p1.y = y1;
    dsc.p2.x = x2; dsc.p2.y = y2;
    lv_draw_line(&layer, &dsc);
    lv_canvas_finish_layer(c, &layer);
}

void renderer_init(lv_obj_t *parent) {
    generate_stars();
    cam.x = LN_START_X;
    cam.y = LN_START_Y;

    // Allocate canvas buffer — try PSRAM first
    size_t buf_size = LN_SCREEN_W * LN_SCREEN_H * 2;
    canvas_buf = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!canvas_buf) {
        canvas_buf = (uint8_t *)malloc(buf_size);
    }

    canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, canvas_buf, LN_SCREEN_W, LN_SCREEN_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(canvas, 0, 0);

    // HUD labels
    lbl_fuel = lv_label_create(parent);
    lv_obj_set_pos(lbl_fuel, 4, 4);
    lv_obj_set_style_text_color(lbl_fuel, lv_color_white(), 0);
    lv_label_set_text(lbl_fuel, "Fuel: 100%");

    lbl_speed = lv_label_create(parent);
    lv_obj_set_pos(lbl_speed, 4, 20);
    lv_obj_set_style_text_color(lbl_speed, lv_color_white(), 0);
    lv_label_set_text(lbl_speed, "Spd: 0.0");

    lbl_alt = lv_label_create(parent);
    lv_obj_set_pos(lbl_alt, 4, 36);
    lv_obj_set_style_text_color(lbl_alt, lv_color_white(), 0);
    lv_label_set_text(lbl_alt, "Alt: 0");

    lbl_warn = lv_label_create(parent);
    lv_obj_align(lbl_warn, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_text_color(lbl_warn, lv_color_make(255, 60, 60), 0);
    lv_label_set_text(lbl_warn, "");
    lv_obj_add_flag(lbl_warn, LV_OBJ_FLAG_HIDDEN);

    // Fuel bar
    bar_fuel = lv_bar_create(parent);
    lv_obj_set_size(bar_fuel, 80, 8);
    lv_obj_set_pos(bar_fuel, LN_SCREEN_W - 84, 4);
    lv_bar_set_range(bar_fuel, 0, 100);
    lv_bar_set_value(bar_fuel, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_fuel, lv_color_make(40, 40, 40), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_fuel, lv_color_make(0, 200, 0), LV_PART_INDICATOR);
}

static void draw_stars() {
    for (int i = 0; i < NUM_STARS; i++) {
        int16_t sx = world_to_screen_x(stars[i][0], cam);
        int16_t sy = world_to_screen_y(stars[i][1], cam);
        if (sx >= 0 && sx < LN_SCREEN_W && sy >= 0 && sy < LN_SCREEN_H) {
            lv_canvas_set_px(canvas, sx, sy, lv_color_white(), LV_OPA_COVER);
        }
    }
}

static void draw_terrain(const Terrain &t) {
    lv_color_t green = lv_color_make(0, 200, 0);
    lv_color_t zone_color = lv_color_make(255, 255, 0);

    for (uint16_t i = 0; i + 1 < t.num_points; i++) {
        int16_t x1 = world_to_screen_x(t.points[i][0], cam);
        int16_t y1 = world_to_screen_y(t.points[i][1], cam);
        int16_t x2 = world_to_screen_x(t.points[i + 1][0], cam);
        int16_t y2 = world_to_screen_y(t.points[i + 1][1], cam);

        // Skip if entirely off screen
        if ((x1 < 0 && x2 < 0) || (x1 >= LN_SCREEN_W && x2 >= LN_SCREEN_W)) continue;

        bool is_zone = (t.points[i][0] >= t.zone_x1 && t.points[i + 1][0] <= t.zone_x2 &&
                        t.points[i][1] == t.zone_y && t.points[i + 1][1] == t.zone_y);
        draw_line(canvas, x1, y1, x2, y2, is_zone ? zone_color : green);
    }
}

static void draw_lander(const Lander &l) {
    float s = sinf(l.rotation);
    float c = cosf(l.rotation);

    // Triangle: nose up, two base corners
    float pts[3][2] = {
        { 0, -LANDER_SIZE},                          // nose
        {-LANDER_SIZE / 2.0f,  LANDER_SIZE / 2.0f},  // left base
        { LANDER_SIZE / 2.0f,  LANDER_SIZE / 2.0f}   // right base
    };

    int16_t sx[3], sy[3];
    for (int i = 0; i < 3; i++) {
        float rx = pts[i][0] * c - pts[i][1] * s;
        float ry = pts[i][0] * s + pts[i][1] * c;
        sx[i] = world_to_screen_x(l.x + rx, cam);
        sy[i] = world_to_screen_y(l.y + ry, cam);
    }

    lv_color_t white = lv_color_white();
    draw_line(canvas, sx[0], sy[0], sx[1], sy[1], white);
    draw_line(canvas, sx[1], sy[1], sx[2], sy[2], white);
    draw_line(canvas, sx[2], sy[2], sx[0], sy[0], white);

    // Thrust flame
    if (l.thrusting && l.fuel > 0) {
        float fx = 0;
        float fy = LANDER_SIZE;
        float rfx = fx * c - fy * s;
        float rfy = fx * s + fy * c;
        int16_t flame_x = world_to_screen_x(l.x + rfx, cam);
        int16_t flame_y = world_to_screen_y(l.y + rfy, cam);
        lv_color_t orange = lv_color_make(255, 140, 0);
        draw_line(canvas, sx[1], sy[1], flame_x, flame_y, orange);
        draw_line(canvas, sx[2], sy[2], flame_x, flame_y, orange);
    }
}

static void update_hud(const GameState &gs) {
    int fuel_pct = (int)(gs.lander.fuel * 100.0f / LN_INITIAL_FUEL);
    if (fuel_pct < 0) fuel_pct = 0;
    lv_bar_set_value(bar_fuel, fuel_pct, LV_ANIM_OFF);

    if (fuel_pct < 25) {
        lv_obj_set_style_bg_color(bar_fuel, lv_color_make(255, 0, 0), LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_bg_color(bar_fuel, lv_color_make(0, 200, 0), LV_PART_INDICATOR);
    }

    float spd = lander_speed(gs.lander);
    float alt = terrain_height_at(gs.terrain, gs.lander.x) - gs.lander.y;
    if (alt < 0) alt = 0;

    lv_label_set_text_fmt(lbl_fuel, "Fuel: %d%%", fuel_pct);
    lv_label_set_text_fmt(lbl_speed, "Spd: %.1f", (double)spd);
    lv_label_set_text_fmt(lbl_alt, "Alt: %d", (int)alt);

    // Warning for high speed near ground
    if (alt < 80 && spd > LN_MAX_LANDING_SPEED) {
        lv_label_set_text(lbl_warn, "! TOO FAST !");
        lv_obj_clear_flag(lbl_warn, LV_OBJ_FLAG_HIDDEN);
    } else if (alt < 80 && fabsf(gs.lander.rotation) > LN_MAX_LANDING_ANGLE) {
        lv_label_set_text(lbl_warn, "! ANGLE !");
        lv_obj_clear_flag(lbl_warn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(lbl_warn, LV_OBJ_FLAG_HIDDEN);
    }
}

void renderer_draw(const GameState &gs) {
    if (!canvas) return;

    camera_update(cam, gs.lander);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    draw_stars();
    draw_terrain(gs.terrain);
    draw_lander(gs.lander);
    update_hud(gs);
}

void renderer_cleanup() {
    if (canvas) { lv_obj_del(canvas); canvas = NULL; }
    if (lbl_fuel) { lv_obj_del(lbl_fuel); lbl_fuel = NULL; }
    if (lbl_speed) { lv_obj_del(lbl_speed); lbl_speed = NULL; }
    if (lbl_alt) { lv_obj_del(lbl_alt); lbl_alt = NULL; }
    if (lbl_warn) { lv_obj_del(lbl_warn); lbl_warn = NULL; }
    if (bar_fuel) { lv_obj_del(bar_fuel); bar_fuel = NULL; }
    if (canvas_buf) { free(canvas_buf); canvas_buf = NULL; }
    stars_generated = false;
}

#endif // NATIVE_TEST
