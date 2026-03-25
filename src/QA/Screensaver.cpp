#include "QA/Screensaver.h"
#include "QA/Menu.h"
#include <lvgl.h>
#include <math.h>

static lv_timer_t *ss_timer = NULL;
static lv_obj_t *ss_screen = NULL;
static lv_obj_t *ss_canvas = NULL;
static uint8_t *ss_buf = NULL;
static bool ss_active = false;

#define SS_NUM_STARS 60
static int16_t star_x[SS_NUM_STARS];
static int16_t star_y[SS_NUM_STARS];
static uint8_t star_speed[SS_NUM_STARS];

static void init_stars() {
    for (int i = 0; i < SS_NUM_STARS; i++) {
        star_x[i] = random(320);
        star_y[i] = random(240);
        star_speed[i] = 1 + random(3);
    }
}

static void ss_tick(lv_timer_t *t) {
    if (!ss_canvas) return;
    lv_canvas_fill_bg(ss_canvas, lv_color_black(), LV_OPA_COVER);
    uint16_t *buf = (uint16_t *)ss_buf;
    for (int i = 0; i < SS_NUM_STARS; i++) {
        star_x[i] -= star_speed[i];
        if (star_x[i] < 0) { star_x[i] = 319; star_y[i] = random(240); }
        uint8_t bright = star_speed[i] == 3 ? 255 : (star_speed[i] == 2 ? 160 : 80);
        uint16_t c565 = ((bright >> 3) << 11) | ((bright >> 2) << 5) | (bright >> 3);
        buf[star_y[i] * 320 + star_x[i]] = c565;
    }
    // Occasional shooting star
    static int shoot_timer = 0;
    static int16_t shoot_x, shoot_y;
    if (shoot_timer <= 0 && random(100) < 3) {
        shoot_x = 319; shoot_y = random(120);
        shoot_timer = 20;
    }
    if (shoot_timer > 0) {
        for (int j = 0; j < 4; j++) {
            int16_t sx = shoot_x - j * 3;
            int16_t sy = shoot_y + j;
            if (sx >= 0 && sx < 320 && sy >= 0 && sy < 240) {
                uint8_t b = 255 - j * 60;
                buf[sy * 320 + sx] = ((b >> 3) << 11) | ((b >> 2) << 5) | (b >> 3);
            }
        }
        shoot_x -= 6; shoot_y += 2;
        shoot_timer--;
    }

    // Earthrise — small Earth rising from bottom-right
    static int16_t earth_y = 260;
    earth_y--;
    if (earth_y < 160) earth_y = 260;
    const int16_t ex = 260, r = 20;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                int px = ex + dx, py = earth_y + dy;
                if (px >= 0 && px < 320 && py >= 0 && py < 240) {
                    bool land = ((dx + dy) % 7 == 0) || ((dx * 3 + dy * 2) % 11 == 0);
                    buf[py * 320 + px] = land ? 0x0600 : 0x001F;
                }
            }
        }
    }
    // Lunar horizon
    for (int x = 200; x < 320; x++)
        if (220 < 240) buf[220 * 320 + x] = 0x4208;

    // Orbital drift - tiny lander orbiting a moon
    static float orbit_angle = 0;
    orbit_angle += 0.02f;
    if (orbit_angle > 6.283f) orbit_angle -= 6.283f;
    // Small moon at center-left
    int mx = 100, my = 120, mr = 8;
    for (int dy = -mr; dy <= mr; dy++) {
        for (int dx = -mr; dx <= mr; dx++) {
            if (dx*dx + dy*dy <= mr*mr) {
                int px = mx+dx, py = my+dy;
                if (px >= 0 && px < 320 && py >= 0 && py < 240)
                    buf[py * 320 + px] = 0x6B4D; // gray moon
            }
        }
    }
    // Lander on elliptical orbit
    float ox = mx + cosf(orbit_angle) * 30;
    float oy = my + sinf(orbit_angle) * 18; // elliptical
    int lx = (int)ox, ly = (int)oy;
    if (lx >= 1 && lx < 319 && ly >= 1 && ly < 239) {
        buf[ly * 320 + lx] = 0xFFFF;       // white center
        buf[ly * 320 + lx - 1] = 0xFFFF;
        buf[ly * 320 + lx + 1] = 0xFFFF;
        buf[(ly-1) * 320 + lx] = 0xFFFF;
    }
}

static void ss_touch_cb(lv_event_t *e) {
    screensaver_stop();
    create_main_menu(false);
}

static void ss_timeout_cb(lv_timer_t *t) {
    lv_timer_del(ss_timer);
    ss_timer = NULL;

    ss_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ss_screen, lv_color_black(), 0);

    size_t buf_size = 320 * 240 * 2;
    ss_buf = (uint8_t *)malloc(buf_size);
    if (!ss_buf) return;

    ss_canvas = lv_canvas_create(ss_screen);
    lv_canvas_set_buffer(ss_canvas, ss_buf, 320, 240, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(ss_canvas, 0, 0);

    lv_obj_add_event_cb(ss_screen, ss_touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_flag(ss_screen, LV_OBJ_FLAG_CLICKABLE);

    load_screen_and_delete_old(ss_screen);
    init_stars();
    ss_active = true;
    ss_timer = lv_timer_create(ss_tick, 50, NULL);
}

void screensaver_reset_timer() {
    if (ss_active) return;
    if (ss_timer) lv_timer_del(ss_timer);
    ss_timer = lv_timer_create(ss_timeout_cb, 60000, NULL);
    lv_timer_set_repeat_count(ss_timer, 1);
}

void screensaver_start_timer() {
    screensaver_reset_timer();
}

void screensaver_stop() {
    if (ss_timer) { lv_timer_del(ss_timer); ss_timer = NULL; }
    if (ss_canvas) { lv_obj_del(ss_canvas); ss_canvas = NULL; }
    if (ss_buf) { free(ss_buf); ss_buf = NULL; }
    ss_screen = NULL;
    ss_active = false;
}
