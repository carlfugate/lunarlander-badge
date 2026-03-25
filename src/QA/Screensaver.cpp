#include "QA/Screensaver.h"
#include "QA/Menu.h"
#include <lvgl.h>
#include <math.h>
#include <string.h>

#ifndef NATIVE_TEST
#include <esp_sleep.h>
#include "pins.h"
#endif

static lv_timer_t *ss_timer = NULL;
static lv_obj_t *ss_screen = NULL;
static lv_obj_t *ss_canvas = NULL;
static uint8_t *ss_buf = NULL;
static bool ss_active = false;
static int ss_total_ticks = 0;
static int ss_scene_ticks = 0;
static int ss_scene_idx = 0;

static ScreensaverMode ss_mode = SS_MODE_AD_ASTRA;

void screensaver_set_mode(ScreensaverMode mode) { ss_mode = mode; }
ScreensaverMode screensaver_get_mode() { return ss_mode; }

#define W 320
#define H 240
#define SCENE_DURATION 600  // 30s at 50ms/tick

// --- Helpers ---
static inline void px(uint16_t *buf, int x, int y, uint16_t c) {
    if (x >= 0 && x < W && y >= 0 && y < H) buf[y * W + x] = c;
}
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}
static inline uint16_t gray565(uint8_t v) { return rgb565(v, v, v); }
static void clear(uint16_t *buf) { memset(buf, 0, W * H * 2); }

// --- Scene 1: Warp Speed ---
static void scene_warp(uint16_t *buf, int tick) {
    clear(buf);
    static float stars[80][3]; // angle, distance, speed
    if (tick == 0) {
        for (int i = 0; i < 80; i++) {
            stars[i][0] = (random(628)) / 100.0f;
            stars[i][1] = random(20) + 5;
            stars[i][2] = 0.5f + (random(20)) / 10.0f;
        }
    }
    int cx = W / 2, cy = H / 2;
    for (int i = 0; i < 80; i++) {
        stars[i][1] += stars[i][2];
        float d = stars[i][1];
        if (d > 200) { stars[i][1] = random(10) + 2; stars[i][0] = random(628) / 100.0f; }
        int x = cx + (int)(cosf(stars[i][0]) * d);
        int y = cy + (int)(sinf(stars[i][0]) * d);
        // Trail
        int trail = (int)(d / 15);
        if (trail > 8) trail = 8;
        for (int t = 0; t <= trail; t++) {
            int tx = cx + (int)(cosf(stars[i][0]) * (d - t * 3));
            int ty = cy + (int)(sinf(stars[i][0]) * (d - t * 3));
            uint8_t b = 255 - t * 28;
            px(buf, tx, ty, gray565(b));
        }
    }
}

// --- Scene 2: Solar System Orrery ---
static void scene_solar(uint16_t *buf, int tick) {
    clear(buf);
    int cx = W / 2, cy = H / 2;
    // Sun
    for (int dy = -4; dy <= 4; dy++)
        for (int dx = -4; dx <= 4; dx++)
            if (dx*dx+dy*dy <= 16) px(buf, cx+dx, cy+dy, rgb565(255, 200, 0));
    // Planets: radius, speed, color
    static const struct { int r; float spd; uint16_t col; } planets[] = {
        {20, 0.08f, 0xC618},  // Mercury gray
        {32, 0.05f, 0xFE60},  // Venus amber
        {46, 0.035f, 0x07FF}, // Earth cyan
        {58, 0.025f, 0xF800}, // Mars red
        {78, 0.012f, 0xFD20}, // Jupiter orange
        {95, 0.008f, 0xFFE0}, // Saturn yellow
    };
    float t = tick * 0.05f;
    for (int i = 0; i < 6; i++) {
        // Dim orbit ring
        for (int a = 0; a < 64; a++) {
            float ang = a * 6.283f / 64;
            px(buf, cx + (int)(cosf(ang) * planets[i].r), cy + (int)(sinf(ang) * planets[i].r), gray565(25));
        }
        float ang = t * planets[i].spd + i * 1.0f;
        int px_ = cx + (int)(cosf(ang) * planets[i].r);
        int py_ = cy + (int)(sinf(ang) * planets[i].r);
        int sz = (i >= 4) ? 2 : 1;
        for (int dy = -sz; dy <= sz; dy++)
            for (int dx = -sz; dx <= sz; dx++)
                px(buf, px_+dx, py_+dy, planets[i].col);
    }
}

// --- Scene 3: Lunar Approach (original starfield + earthrise + orbiter) ---
static struct { int16_t x, y; uint8_t spd; } ss_stars[60];
static void scene_lunar(uint16_t *buf, int tick) {
    clear(buf);
    if (tick == 0) {
        for (int i = 0; i < 60; i++) {
            ss_stars[i].x = random(W); ss_stars[i].y = random(H);
            ss_stars[i].spd = 1 + random(3);
        }
    }
    for (int i = 0; i < 60; i++) {
        ss_stars[i].x -= ss_stars[i].spd;
        if (ss_stars[i].x < 0) { ss_stars[i].x = W-1; ss_stars[i].y = random(H); }
        px(buf, ss_stars[i].x, ss_stars[i].y, gray565(ss_stars[i].spd == 3 ? 255 : ss_stars[i].spd == 2 ? 160 : 80));
    }
    // Earthrise
    static int16_t ey = 260;
    ey--; if (ey < 160) ey = 260;
    for (int dy = -20; dy <= 20; dy++)
        for (int dx = -20; dx <= 20; dx++)
            if (dx*dx+dy*dy <= 400) {
                int px_ = 260+dx, py_ = ey+dy;
                if (px_ >= 0 && px_ < W && py_ >= 0 && py_ < H) {
                    bool land = ((dx+dy)%7==0) || ((dx*3+dy*2)%11==0);
                    buf[py_*W+px_] = land ? 0x0600 : 0x001F;
                }
            }
    if (ey < H) for (int x = 200; x < W; x++) px(buf, x, 220, 0x4208);
    // Orbiter
    static float oa = 0; oa += 0.02f; if (oa > 6.283f) oa -= 6.283f;
    for (int dy = -8; dy <= 8; dy++)
        for (int dx = -8; dx <= 8; dx++)
            if (dx*dx+dy*dy <= 64) px(buf, 100+dx, 120+dy, 0x6B4D);
    int lx = 100+(int)(cosf(oa)*30), ly = 120+(int)(sinf(oa)*18);
    px(buf, lx, ly, 0xFFFF); px(buf, lx-1, ly, 0xFFFF); px(buf, lx+1, ly, 0xFFFF); px(buf, lx, ly-1, 0xFFFF);
}

// --- Scene 4: Apollo Descent ---
static void scene_descent(uint16_t *buf, int tick) {
    clear(buf);
    // Terrain
    for (int x = 0; x < W; x++) {
        int ty = 200 + (int)(sinf(x * 0.03f) * 15 + sinf(x * 0.07f) * 8);
        // Flat landing zone 140-180
        if (x >= 140 && x <= 180) ty = 200;
        for (int y = ty; y < H; y++) px(buf, x, y, rgb565(15, 30, 15));
        px(buf, x, ty, rgb565(0, 180, 0));
        if (x >= 140 && x <= 180) px(buf, x, ty, rgb565(255, 255, 0));
    }
    // Lander descending
    float progress = (tick % 400) / 400.0f; // 0->1 over 20s
    int lx = 60 + (int)(progress * 100);
    int ly = 40 + (int)(progress * 150);
    if (ly > 192) ly = 192;
    // Body
    for (int dy = -3; dy <= 0; dy++)
        for (int dx = -2; dx <= 2; dx++)
            px(buf, lx+dx, ly+dy, 0xFFFF);
    // Legs
    px(buf, lx-3, ly+1, gray565(180)); px(buf, lx+3, ly+1, gray565(180));
    px(buf, lx-4, ly+2, gray565(180)); px(buf, lx+4, ly+2, gray565(180));
    // Thrust flame (flicker)
    if (progress < 0.9f && (tick % 4 < 2)) {
        px(buf, lx, ly+3, rgb565(255, 140, 0));
        px(buf, lx-1, ly+2, rgb565(255, 80, 0));
        px(buf, lx+1, ly+2, rgb565(255, 80, 0));
    }
    // Stars
    for (int i = 0; i < 30; i++) {
        int sx = (i * 97 + tick) % W, sy = (i * 53) % 180;
        px(buf, sx, sy, gray565(60 + (i % 3) * 40));
    }
}

// --- Scene 5: Pulsar ---
static void scene_pulsar(uint16_t *buf, int tick) {
    clear(buf);
    // Background stars
    for (int i = 0; i < 40; i++) {
        int sx = (i * 97) % W, sy = (i * 53) % H;
        px(buf, sx, sy, gray565(40));
    }
    int cx = W/2, cy = H/2;
    // Core
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++)
            if (dx*dx+dy*dy <= 4) px(buf, cx+dx, cy+dy, 0xFFFF);
    // Rotating beams
    float ang = tick * 0.06f;
    for (int beam = 0; beam < 2; beam++) {
        float a = ang + beam * 3.14159f;
        for (int d = 5; d < 160; d++) {
            int bx = cx + (int)(cosf(a) * d);
            int by = cy + (int)(sinf(a) * d);
            uint8_t b = 200 - d;
            if (b < 20) b = 20;
            px(buf, bx, by, rgb565(0, b, b + 40));
            px(buf, bx, by+1, rgb565(0, b/2, b/2 + 20));
        }
    }
    // Stars brighten when beam passes
    for (int i = 0; i < 40; i++) {
        int sx = (i * 97) % W, sy = (i * 53) % H;
        float sa = atan2f(sy - cy, sx - cx);
        float diff = fmodf(fabsf(sa - fmodf(ang, 6.283f)), 6.283f);
        if (diff > 3.14159f) diff = 6.283f - diff;
        if (diff < 0.15f) px(buf, sx, sy, 0xFFFF);
    }
}

// --- Scene 6: Matrix Rain ---
static void scene_matrix(uint16_t *buf, int tick) {
    // Don't clear — let trails fade
    // Dim entire screen by shifting green channel down
    for (int i = 0; i < W * H; i++) {
        uint16_t c = buf[i];
        uint8_t g = (c >> 5) & 0x3F;
        if (g > 2) g -= 2; else g = 0;
        buf[i] = (g << 5);
    }
    // Falling columns
    static int16_t col_y[32];
    static uint8_t col_speed[32];
    if (tick == 0) {
        for (int i = 0; i < 32; i++) {
            col_y[i] = -(random(H));
            col_speed[i] = 2 + random(4);
        }
    }
    for (int i = 0; i < 32; i++) {
        col_y[i] += col_speed[i];
        if (col_y[i] > H + 20) { col_y[i] = -(random(40)); col_speed[i] = 2 + random(4); }
        int x = i * 10 + 5;
        // Head (bright)
        px(buf, x, col_y[i], rgb565(180, 255, 180));
        // Trail chars
        for (int t = 1; t < 8; t++) {
            int ty = col_y[i] - t * 3;
            uint8_t g = 200 - t * 25;
            px(buf, x, ty, rgb565(0, g, 0));
            // Fake char: offset pixel
            px(buf, x + (((i+t+tick) * 7) % 3) - 1, ty, rgb565(0, g/2, 0));
        }
    }
}

// --- Scene 7: Terminal Boot ---
static void scene_terminal(uint16_t *buf, int tick) {
    clear(buf);
    static const char* lines[] = {
        "[OK] Initializing crypto engine...",
        "[OK] Loading kernel modules...",
        "[OK] Firewall rules: 47 active",
        "[OK] Ad Astra Protocol v2.6",
        "[OK] Badge mesh: scanning...",
        "[WARN] Coffee supply: LOW",
        "[OK] CTF engine: armed",
        "[OK] NeoPixel array: 6 online",
        "[OK] Lunar guidance: nominal",
        "[OK] BSidesKC link: established",
        "[OK] Comms: encrypted",
        "[WARN] Snack reserves: depleted",
        "[OK] Badge registered",
        "[OK] All systems GO",
        "> LAUNCH COMMIT _",
    };
    int num_lines = sizeof(lines) / sizeof(lines[0]);
    int visible = (tick / 10) + 1; // new line every 500ms
    if (visible > num_lines) visible = num_lines;
    int start = visible > 14 ? visible - 14 : 0; // scroll if needed
    for (int i = start; i < visible; i++) {
        int y = (i - start) * 16 + 8;
        const char *line = lines[i];
        // Render char by char as colored pixels
        bool is_warn = (line[1] == 'W');
        uint16_t col = is_warn ? rgb565(255, 170, 0) : rgb565(0, 200, 0);
        if (line[0] == '>') col = rgb565(0, 229, 255); // cyan prompt
        for (int c = 0; line[c] && c < 40; c++) {
            if (line[c] == ' ') continue;
            // Simple 3x5 block per char
            int cx = 8 + c * 7;
            for (int py = 0; py < 5; py++)
                for (int ppx = 0; ppx < 3; ppx++)
                    if ((line[c] + py + ppx) % 3 != 0) // pseudo-glyph
                        px(buf, cx + ppx, y + py, col);
        }
    }
    // Blinking cursor
    if ((tick / 10) % 2 == 0 && visible >= num_lines) {
        int cy = (num_lines - 1 - (visible > 14 ? visible - 14 : 0)) * 16 + 8;
        for (int py = 0; py < 5; py++)
            for (int ppx = 0; ppx < 3; ppx++)
                px(buf, 8 + 38 * 7 + ppx, cy + py, rgb565(0, 229, 255));
    }
}

// --- Scene 8: Lava Lamp ---
static void scene_lava(uint16_t *buf, int tick) {
    static float blobs[4][3]; // x, y, phase
    if (tick == 0) {
        for (int i = 0; i < 4; i++) {
            blobs[i][0] = 80 + random(160);
            blobs[i][1] = 60 + random(120);
            blobs[i][2] = random(628) / 100.0f;
        }
    }
    // Move blobs
    for (int i = 0; i < 4; i++) {
        blobs[i][2] += 0.02f;
        blobs[i][0] += sinf(blobs[i][2] + i) * 0.8f;
        blobs[i][1] += cosf(blobs[i][2] * 0.7f + i * 2) * 0.6f;
        if (blobs[i][0] < 40) blobs[i][0] = 40;
        if (blobs[i][0] > 280) blobs[i][0] = 280;
        if (blobs[i][1] < 30) blobs[i][1] = 30;
        if (blobs[i][1] > 210) blobs[i][1] = 210;
    }
    // Render metaballs
    for (int y = 0; y < H; y += 2) { // skip rows for speed
        for (int x = 0; x < W; x += 2) {
            float sum = 0;
            for (int i = 0; i < 4; i++) {
                float dx = x - blobs[i][0];
                float dy = y - blobs[i][1];
                float d2 = dx*dx + dy*dy + 1;
                sum += 800.0f / d2;
            }
            if (sum > 0.8f) {
                uint8_t r, g, b;
                if (sum > 1.5f) { r = 255; g = 100; b = 0; }      // hot center
                else if (sum > 1.2f) { r = 200; g = 50; b = 100; } // warm
                else { r = 100; g = 0; b = 150; }                  // cool edge
                uint16_t c = rgb565(r, g, b);
                buf[y * W + x] = c;
                buf[y * W + x + 1] = c;
                if (y + 1 < H) { buf[(y+1) * W + x] = c; buf[(y+1) * W + x + 1] = c; }
            } else {
                buf[y * W + x] = 0;
                buf[y * W + x + 1] = 0;
                if (y + 1 < H) { buf[(y+1) * W + x] = 0; buf[(y+1) * W + x + 1] = 0; }
            }
        }
    }
}

// --- Scene 9: Starfield with touch ripples ---
static int16_t ripple_x = -1, ripple_y = -1, ripple_r = 0;
static void scene_ripple(uint16_t *buf, int tick) {
    clear(buf);
    // Stars
    for (int i = 0; i < 50; i++) {
        int sx = (i * 97 + tick / 3) % W, sy = (i * 53) % H;
        px(buf, sx, sy, gray565(60 + (i % 4) * 30));
    }
    // Ripple
    if (ripple_r > 0) {
        uint8_t bright = ripple_r < 60 ? (uint8_t)(200 - ripple_r * 3) : 20;
        for (int a = 0; a < 64; a++) {
            float ang = a * 6.283f / 64;
            int rx = ripple_x + (int)(cosf(ang) * ripple_r);
            int ry = ripple_y + (int)(sinf(ang) * ripple_r);
            px(buf, rx, ry, rgb565(0, bright, bright + 40));
        }
        ripple_r += 3;
        if (ripple_r > 80) ripple_r = 0;
    }
    // "Touch me" hint
    if (tick < 100 && (tick / 15) % 2 == 0) {
        // Small dot cluster at center
        px(buf, W/2, H/2, rgb565(0, 100, 120));
        px(buf, W/2-1, H/2, rgb565(0, 60, 80));
        px(buf, W/2+1, H/2, rgb565(0, 60, 80));
    }
}

// --- Scene table: Ad Astra story arc ---
typedef void (*scene_fn)(uint16_t *buf, int tick);
static const scene_fn story_scenes[] = {
    scene_warp,      // Departing Earth
    scene_solar,     // Passing through the solar system
    scene_lunar,     // Arrived — earthrise + orbiter
    scene_descent,   // The landing mission
    scene_pulsar,    // Deep space beacon
    scene_ripple,    // Interactive starfield
};
static const int NUM_STORY_SCENES = sizeof(story_scenes) / sizeof(story_scenes[0]);

// --- Main tick ---
static void ss_tick(lv_timer_t *t) {
    if (!ss_canvas || !ss_buf) return;

#ifndef NATIVE_TEST
    ss_total_ticks++;
    // Deep sleep disabled — reboot loses all state (achievements, callsign loaded from SD).
    // The screensaver itself is low-power enough for a conference day.
#endif

    uint16_t *buf = (uint16_t *)ss_buf;

    if (ss_mode == SS_MODE_AD_ASTRA) {
        story_scenes[ss_scene_idx](buf, ss_scene_ticks);
        ss_scene_ticks++;
        if (ss_scene_ticks >= SCENE_DURATION) {
            ss_scene_ticks = 0;
            ss_scene_idx = (ss_scene_idx + 1) % NUM_STORY_SCENES;
        }
    } else if (ss_mode == SS_MODE_MATRIX) {
        scene_matrix(buf, ss_scene_ticks++);
    } else if (ss_mode == SS_MODE_TERMINAL) {
        scene_terminal(buf, ss_scene_ticks++);
    } else if (ss_mode == SS_MODE_LAVA) {
        scene_lava(buf, ss_scene_ticks++);
    }

    lv_obj_invalidate(ss_canvas); // tell LVGL the canvas changed
}

static void ss_touch_cb(lv_event_t *e) {
    // Ripple scene: create ripple on touch instead of waking
    if (ss_mode == SS_MODE_AD_ASTRA && ss_scene_idx == NUM_STORY_SCENES - 1) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_active(), &p);
        ripple_x = p.x; ripple_y = p.y; ripple_r = 1;
        return;
    }
    screensaver_stop();
    create_main_menu(false);
}

static void ss_timeout_cb(lv_timer_t *t) {
    lv_timer_del(ss_timer);
    ss_timer = NULL;

    ss_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ss_screen, lv_color_black(), 0);

    size_t buf_size = W * H * 2;
    ss_buf = (uint8_t *)malloc(buf_size);
    if (!ss_buf) return;
    memset(ss_buf, 0, buf_size);

    ss_canvas = lv_canvas_create(ss_screen);
    lv_canvas_set_buffer(ss_canvas, ss_buf, W, H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(ss_canvas, 0, 0);

    lv_obj_add_event_cb(ss_screen, ss_touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_flag(ss_screen, LV_OBJ_FLAG_CLICKABLE);

    stop_menu_timers();
    load_screen_and_delete_old(ss_screen);
    ss_active = true;
    ss_total_ticks = 0;
    ss_scene_ticks = 0;
    ss_scene_idx = 0;
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
    ss_canvas = NULL;
    if (ss_buf) { free(ss_buf); ss_buf = NULL; }
    ss_screen = NULL;
    ss_active = false;
}

#ifdef FF_SERIAL_TEST
#include "QA/SerialCmd.h"
static void screensaver_serial_handler(const char *args) {
    if (strncmp(args, "mode ", 5) == 0) {
        int m = atoi(args + 5);
        screensaver_set_mode((ScreensaverMode)m);
        serial_cmd_log("SCREENSAVER", "mode=%d", m);
    } else if (strcmp(args, "trigger") == 0) {
        screensaver_stop();
        screensaver_reset_timer();
        serial_cmd_log("SCREENSAVER", "timer_reset");
    } else if (strcmp(args, "status") == 0) {
        serial_cmd_log("SCREENSAVER", "mode=%d", (int)screensaver_get_mode());
    } else if (strcmp(args, "list") == 0) {
        serial_cmd_log("SCREENSAVER", "0=ad_astra 1=matrix 2=terminal 3=lava");
    } else {
        serial_cmd_log("SCREENSAVER", "error=unknown args=%s", args);
    }
}
void serial_register_screensaver() {
    serial_cmd_register("screensaver", screensaver_serial_handler, "mode <n>, trigger, status, list");
}
#else
void serial_register_screensaver() {}
#endif
