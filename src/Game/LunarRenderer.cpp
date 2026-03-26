#include "Game/LunarRenderer.h"
#include "Game/LunarConfig.h"
#include <math.h>

// Camera functions — always compiled (used by simulator and tests)
// Full-world view: entire 1200x800 world scaled to 320x240 screen
void camera_update(Camera &cam, const Lander &l, int phase) {
    // No-op: full world always visible, matching web/mobile behavior
    (void)cam; (void)l; (void)phase;
}

int16_t world_to_screen_x(float wx, const Camera &cam) {
    (void)cam;
    return (int16_t)(wx * LN_SCALE_X);
}

int16_t world_to_screen_y(float wy, const Camera &cam) {
    (void)cam;
    return (int16_t)(wy * LN_SCALE_Y);
}

#ifndef NATIVE_TEST

#include <esp_heap_caps.h>
#include <Arduino.h>

#define NUM_STARS 40
#define LANDER_SIZE 8

static lv_obj_t *canvas = NULL;
static uint8_t *canvas_buf = NULL;
static Camera cam;

// HUD elements
static lv_obj_t *lbl_fuel = NULL;
static lv_obj_t *lbl_speed = NULL;
static lv_obj_t *lbl_alt = NULL;
static lv_obj_t *lbl_time = NULL;
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
    cam = {0, 0, 0, 0, 0};

    // Allocate canvas buffer — try PSRAM first
    size_t buf_size = LN_SCREEN_W * LN_SCREEN_H * 2;
    Serial.printf("[RENDERER] need=%d heap=%d max_block=%d\n", buf_size, ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    canvas_buf = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!canvas_buf) {
        canvas_buf = (uint8_t *)malloc(buf_size);
    }
    Serial.printf("[RENDERER] canvas_buf=%p heap=%d\n", canvas_buf, ESP.getFreeHeap());
    if (!canvas_buf) {
        Serial.println("[RENDERER] FAILED - not enough contiguous memory");
        return;
    }

    canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, canvas_buf, LN_SCREEN_W, LN_SCREEN_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(canvas, 0, 0);

    // HUD labels (monospace font for stable numeric display)
    lbl_fuel = lv_label_create(parent);
    lv_obj_set_pos(lbl_fuel, 4, 4);
    lv_obj_set_style_text_color(lbl_fuel, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_fuel, &lv_font_unscii_8, 0);
    lv_label_set_text(lbl_fuel, "Fuel: 100%");

    lbl_speed = lv_label_create(parent);
    lv_obj_set_pos(lbl_speed, 4, 20);
    lv_obj_set_style_text_color(lbl_speed, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_speed, &lv_font_unscii_8, 0);
    lv_label_set_text(lbl_speed, "Spd: 0.0");

    lbl_alt = lv_label_create(parent);
    lv_obj_set_pos(lbl_alt, 4, 36);
    lv_obj_set_style_text_color(lbl_alt, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_alt, &lv_font_unscii_8, 0);
    lv_label_set_text(lbl_alt, "Alt: 0");

    lbl_time = lv_label_create(parent);
    lv_obj_align(lbl_time, LV_ALIGN_TOP_RIGHT, -4, 28);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_unscii_8, 0);
    lv_label_set_text(lbl_time, "0.00s");

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

static void fill_terrain(const Terrain &t) {
    // RGB565: R=20>>3=2, G=40>>2=10, B=20>>3=2 → 0x0542
    const uint16_t fill565 = (2 << 11) | (10 << 5) | 2;
    uint16_t *buf = (uint16_t *)canvas_buf;
    const int stride = LN_SCREEN_W;
    for (uint16_t i = 0; i + 1 < t.num_points; i++) {
        int16_t x1 = world_to_screen_x(t.points[i][0], cam);
        int16_t y1 = world_to_screen_y(t.points[i][1], cam);
        int16_t x2 = world_to_screen_x(t.points[i + 1][0], cam);
        int16_t y2 = world_to_screen_y(t.points[i + 1][1], cam);
        if (x1 > x2) { int16_t tmp; tmp=x1; x1=x2; x2=tmp; tmp=y1; y1=y2; y2=tmp; }
        if (x2 < 0 || x1 >= LN_SCREEN_W) continue;
        int16_t sx = x1 < 0 ? 0 : x1;
        int16_t ex = x2 >= LN_SCREEN_W ? LN_SCREEN_W - 1 : x2;
        for (int16_t x = sx; x <= ex; x++) {
            int16_t y = (x2 == x1) ? y1 : y1 + (y2 - y1) * (x - x1) / (x2 - x1);
            if (y < 0) y = 0;
            if (y >= LN_SCREEN_H) continue;
            for (int16_t row = y; row < LN_SCREEN_H; row++)
                buf[row * stride + x] = fill565;
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
        // Draw landing zone indicator just below terrain surface
        if (is_zone) {
            draw_line(canvas, x1, y1 + 1, x2, y2 + 1, zone_color);
            draw_line(canvas, x1, y1 + 2, x2, y2 + 2, zone_color);
            draw_line(canvas, x1, y1 + 3, x2, y2 + 3, lv_color_make(255, 200, 0));
        }
    }
}

static void draw_zone_indicator(const Terrain &t) {
    // Show arrow pointing toward landing zone when it's off-screen
    int16_t zx_mid = (t.zone_x1 + t.zone_x2) / 2;
    int16_t sx = world_to_screen_x(zx_mid, cam);
    int16_t sy = world_to_screen_y(t.zone_y, cam);
    // If zone is on screen, skip
    if (sx >= 0 && sx < LN_SCREEN_W && sy >= 0 && sy < LN_SCREEN_H) return;

    lv_color_t yellow = lv_color_make(255, 255, 0);
    // Clamp indicator to screen edges
    int16_t ix = sx < 0 ? 8 : (sx >= LN_SCREEN_W ? LN_SCREEN_W - 8 : sx);
    int16_t iy = sy < 0 ? 8 : (sy >= LN_SCREEN_H ? LN_SCREEN_H - 20 : sy);
    // Draw small diamond marker
    draw_line(canvas, ix, iy - 4, ix + 4, iy, yellow);
    draw_line(canvas, ix + 4, iy, ix, iy + 4, yellow);
    draw_line(canvas, ix, iy + 4, ix - 4, iy, yellow);
    draw_line(canvas, ix - 4, iy, ix, iy - 4, yellow);
}

static void draw_touch_controls() {
    lv_color_t dim = lv_color_make(60, 60, 60);
    // Vertical divider at mid-width (full height)
    draw_line(canvas, LN_SCREEN_W / 2, 0, LN_SCREEN_W / 2, LN_SCREEN_H - 1, dim);
    // Horizontal divider on right half only
    draw_line(canvas, LN_SCREEN_W / 2, LN_SCREEN_H / 2, LN_SCREEN_W - 1, LN_SCREEN_H / 2, dim);

    // Left zone: up-arrow (thrust) at center of left half
    int16_t lx = LN_SCREEN_W / 4;
    int16_t ly = LN_SCREEN_H / 2;
    draw_line(canvas, lx, ly - 6, lx - 6, ly, dim);
    draw_line(canvas, lx, ly - 6, lx + 6, ly, dim);
    // Top-right: left-arrow (rotate CCW)
    int16_t trx = LN_SCREEN_W * 3 / 4;
    int16_t try_ = LN_SCREEN_H / 4;
    draw_line(canvas, trx - 6, try_, trx, try_ - 4, dim);
    draw_line(canvas, trx - 6, try_, trx, try_ + 4, dim);
    // Bottom-right: right-arrow (rotate CW)
    int16_t brx = LN_SCREEN_W * 3 / 4;
    int16_t bry = LN_SCREEN_H * 3 / 4;
    draw_line(canvas, brx + 6, bry, brx, bry - 4, dim);
    draw_line(canvas, brx + 6, bry, brx, bry + 4, dim);
}

static LanderSkin current_skin = SKIN_DEFAULT;

static lv_color_t skin_color(LanderSkin skin, bool crashed) {
    if (crashed) return lv_color_make(255, 0, 0);
    switch (skin) {
        case SKIN_GOLD: return lv_color_make(255, 200, 0);
        case SKIN_RED: return lv_color_make(255, 60, 60);
        case SKIN_RAINBOW: {
            static uint8_t hue = 0;
            hue += 5;
            if (hue < 85) return lv_color_make(255 - hue*3, hue*3, 0);
            if (hue < 170) return lv_color_make(0, 255-(hue-85)*3, (hue-85)*3);
            return lv_color_make((hue-170)*3, 0, 255-(hue-170)*3);
        }
        default: return lv_color_white();
    }
}

// Rotate local point by lander rotation, translate to world, project to screen
static void lander_pt(const Lander &l, float lx, float ly, float s, float c, int16_t &sx, int16_t &sy) {
    float wx = l.x + lx * c - ly * s;
    float wy = l.y + lx * s + ly * c;
    sx = world_to_screen_x(wx, cam);
    sy = world_to_screen_y(wy, cam);
}

static void draw_lander(const Lander &l) {
    float s = sinf(l.rotation);
    float c = cosf(l.rotation);
    lv_color_t col = skin_color(current_skin, l.crashed);
    lv_color_t col2 = l.crashed ? lv_color_make(255, 0, 0) : lv_color_make(180, 180, 180);

    // Lunar module shape (matching web version, in world coords)
    // Body: rect from (-12,-25) to (12,-5)
    int16_t bx[4], by[4];
    lander_pt(l, -12, -25, s, c, bx[0], by[0]);
    lander_pt(l,  12, -25, s, c, bx[1], by[1]);
    lander_pt(l,  12,  -5, s, c, bx[2], by[2]);
    lander_pt(l, -12,  -5, s, c, bx[3], by[3]);
    for (int i = 0; i < 4; i++) {
        int ni = (i + 1) % 4;
        draw_line(canvas, bx[i], by[i], bx[ni], by[ni], col);
    }

    // Command module: rect from (-8,-30) to (8,-25)
    int16_t cx[4], cy[4];
    lander_pt(l, -8, -30, s, c, cx[0], cy[0]);
    lander_pt(l,  8, -30, s, c, cx[1], cy[1]);
    lander_pt(l,  8, -25, s, c, cx[2], cy[2]);
    lander_pt(l, -8, -25, s, c, cx[3], cy[3]);
    for (int i = 0; i < 4; i++) {
        int ni = (i + 1) % 4;
        draw_line(canvas, cx[i], cy[i], cx[ni], cy[ni], col);
    }

    // Landing legs
    int16_t lx1, ly1, lx2, ly2, lx3, ly3;
    // Left leg
    lander_pt(l, -10, -5, s, c, lx1, ly1);
    lander_pt(l, -16,  0, s, c, lx2, ly2);
    lander_pt(l, -18,  0, s, c, lx3, ly3);
    draw_line(canvas, lx1, ly1, lx2, ly2, col2);
    draw_line(canvas, lx2, ly2, lx3, ly3, col2);
    // Right leg
    lander_pt(l,  10, -5, s, c, lx1, ly1);
    lander_pt(l,  16,  0, s, c, lx2, ly2);
    lander_pt(l,  18,  0, s, c, lx3, ly3);
    draw_line(canvas, lx1, ly1, lx2, ly2, col2);
    draw_line(canvas, lx2, ly2, lx3, ly3, col2);

    // Thrust flame
    if (l.thrusting && l.fuel > 0) {
        int16_t fx1, fy1, fx2, fy2, ftx, fty;
        lander_pt(l, -5, -5, s, c, fx1, fy1);
        lander_pt(l,  5, -5, s, c, fx2, fy2);
        lander_pt(l,  0,  6, s, c, ftx, fty);
        lv_color_t orange = lv_color_make(255, 140, 0);
        draw_line(canvas, fx1, fy1, ftx, fty, orange);
        draw_line(canvas, fx2, fy2, ftx, fty, orange);
    }

    // Bright center dot for visibility at small scale
    int16_t cdx = world_to_screen_x(l.x, cam);
    int16_t cdy = world_to_screen_y(l.y - 15, cam);
    lv_color_t bright = l.crashed ? lv_color_make(255, 0, 0) : lv_color_make(255, 255, 255);
    if (cdx >= 1 && cdx < LN_SCREEN_W - 1 && cdy >= 1 && cdy < LN_SCREEN_H - 1) {
        lv_canvas_set_px(canvas, cdx, cdy, bright, LV_OPA_COVER);
        lv_canvas_set_px(canvas, cdx-1, cdy, bright, LV_OPA_COVER);
        lv_canvas_set_px(canvas, cdx+1, cdy, bright, LV_OPA_COVER);
        lv_canvas_set_px(canvas, cdx, cdy-1, bright, LV_OPA_COVER);
        lv_canvas_set_px(canvas, cdx, cdy+1, bright, LV_OPA_COVER);
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

    float vy = gs.lander.vy;
    float spd = lander_speed(gs.lander);
    float alt = terrain_height_at(gs.terrain, gs.lander.x) - gs.lander.y;
    if (alt < 0) alt = 0;

    int spd_whole = (int)spd;
    int spd_frac = ((int)(spd * 10.0f)) % 10;
    if (spd_frac < 0) spd_frac = -spd_frac;

    lv_label_set_text_fmt(lbl_fuel, "Fuel: %d%%", fuel_pct);

    if (vy > 1.0f) {
        lv_obj_set_style_text_color(lbl_speed, lv_color_make(255, 60, 60), 0);
        lv_label_set_text_fmt(lbl_speed, "Spd: %d.%d v", spd_whole, spd_frac);
    } else if (vy < -1.0f) {
        lv_obj_set_style_text_color(lbl_speed, lv_color_make(60, 255, 60), 0);
        lv_label_set_text_fmt(lbl_speed, "Spd: %d.%d ^", spd_whole, spd_frac);
    } else {
        lv_obj_set_style_text_color(lbl_speed, lv_color_white(), 0);
        lv_label_set_text_fmt(lbl_speed, "Spd: %d.%d", spd_whole, spd_frac);
    }

    lv_label_set_text_fmt(lbl_alt, "Alt: %d", (int)alt);

    // Speed run timer
    uint32_t secs = gs.elapsed_ms / 1000;
    uint32_t ms = (gs.elapsed_ms % 1000) / 10;
    lv_label_set_text_fmt(lbl_time, "%d.%02ds", secs, ms);

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

    camera_update(cam, gs.lander, gs.phase);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    if (gs.phase == PHASE_WAITING) {
        if (lbl_warn) {
            lv_label_set_text(lbl_warn, "Connecting...");
            lv_obj_set_style_text_color(lbl_warn, lv_color_make(0, 229, 255), 0);
            lv_obj_clear_flag(lbl_warn, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    draw_stars();
    fill_terrain(gs.terrain);
    draw_terrain(gs.terrain);
    draw_touch_controls();
    current_skin = gs.skin;
    draw_lander(gs.lander);
    update_hud(gs);

    // CRT scanline overlay — subtract ~12.5% brightness on even rows
    uint16_t *buf = (uint16_t *)canvas_buf;
    for (int y = 0; y < LN_SCREEN_H; y += 2) {
        for (int x = 0; x < LN_SCREEN_W; x++) {
            uint16_t &px = buf[y * LN_SCREEN_W + x];
            px = px - ((px >> 3) & 0x18E3);
        }
    }
}

void renderer_cleanup() {
    // Don't lv_obj_del individual widgets — they're children of game_screen
    // and will be deleted when the screen is deleted by load_screen_and_delete_old.
    // Just null our pointers and free the raw buffer.
    canvas = NULL;
    lbl_fuel = NULL;
    lbl_speed = NULL;
    lbl_alt = NULL;
    lbl_time = NULL;
    lbl_warn = NULL;
    bar_fuel = NULL;
    if (canvas_buf) { free(canvas_buf); canvas_buf = NULL; }
    stars_generated = false;
}

#endif // NATIVE_TEST
