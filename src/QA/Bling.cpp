#include "QA/Bling.hpp"
#include "QA/Menu.h"
#include "Includes.h"
#include "Hardware/NeoPixelControl.h"
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>

static Ticker bling_ticker;
static int bling_mode = 0; // 0=none, 1=rainbow, 2=police, 3=all blink, 4=chase, 5=random, 6=breathe, 7=aurora

static void bling_animate();

// --- Bling effect implementations ---

static void bling_rainbow_cycle() {
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        setNeoPixelColor(i, Adafruit_NeoPixel::ColorHSV((i * 65536L / NUM_NEOPIXELS), 255, 128));
    }
}

static const int police_left_leds[] = {4, 5, 0}; 
static const int police_right_leds[] = {1, 2, 3};
static const int RED_LED_COLOR = Adafruit_NeoPixel::Color(255, 0, 0);
static const int BLUE_LED_COLOR = Adafruit_NeoPixel::Color(0, 0, 255);
static int state = 0;
static void bling_police_lights() {
    state = (state + 1) % 4; // 0=left on, 1=off, 2=right on, 3=off

    // Turn all off by default
    for (int i = 0; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, 0);

    if (state == 0) {
        setNeoPixelColor(police_left_leds[0], RED_LED_COLOR);
        setNeoPixelColor(police_left_leds[1], BLUE_LED_COLOR);
        setNeoPixelColor(police_left_leds[2], RED_LED_COLOR);
    } else if (state == 1) {
        setNeoPixelColor(police_right_leds[0], BLUE_LED_COLOR);
        setNeoPixelColor(police_right_leds[1], RED_LED_COLOR);
        setNeoPixelColor(police_right_leds[2], BLUE_LED_COLOR);
    } else if (state == 2) {
        setNeoPixelColor(police_left_leds[0], BLUE_LED_COLOR);
        setNeoPixelColor(police_left_leds[1], RED_LED_COLOR);
        setNeoPixelColor(police_left_leds[2], BLUE_LED_COLOR);
    } else if (state == 3) {
        setNeoPixelColor(police_right_leds[0], RED_LED_COLOR);
        setNeoPixelColor(police_right_leds[1], BLUE_LED_COLOR);
        setNeoPixelColor(police_right_leds[2], RED_LED_COLOR);
    }
}

static void bling_all_blink() {
    static bool on = false;
    on = !on;
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        setNeoPixelColor(i, on ? Adafruit_NeoPixel::Color(255,255,255) : 0);
    }
}

static void bling_chase() {
    static int pos = 0;
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        setNeoPixelColor(i, (i == pos) ? Adafruit_NeoPixel::Color(0,255,0) : 0);
    }
    pos = (pos + 1) % NUM_NEOPIXELS;
}

static void bling_random() {
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        setNeoPixelColor(i, Adafruit_NeoPixel::Color(random(256), random(256), random(256)));
    }
}

// Ambient breathing - slow dim blue pulse
static void bling_breathing() {
    static uint8_t phase = 0;
    phase += 2;
    uint8_t val = phase < 128 ? phase / 4 : (255 - phase) / 4;
    uint32_t c = Adafruit_NeoPixel::Color(0, 0, val);
    for (int i = 0; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, c);
}

// Aurora - slow HSV wave across LEDs
static void bling_aurora() {
    static uint16_t hue_offset = 0;
    hue_offset += 256;
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        uint16_t hue = hue_offset + i * 8000;
        uint16_t mapped = 21845 + (hue % 21845);
        setNeoPixelColor(i, Adafruit_NeoPixel::ColorHSV(mapped, 255, 25));
    }
}

// Comet - single bright LED chases with fading tail
static void bling_comet() {
    static int pos = 0;
    pos = (pos + 1) % (NUM_NEOPIXELS * 3);
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        int dist = (pos / 3) - i;
        if (dist < 0) dist += NUM_NEOPIXELS;
        if (dist == 0) setNeoPixelColor(i, Adafruit_NeoPixel::Color(255, 255, 255));
        else if (dist == 1) setNeoPixelColor(i, Adafruit_NeoPixel::Color(100, 100, 255));
        else if (dist == 2) setNeoPixelColor(i, Adafruit_NeoPixel::Color(20, 20, 80));
        else setNeoPixelColor(i, 0);
    }
}

// Fire - flickering warm colors
static void bling_fire() {
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        uint8_t r = 180 + random(75);
        uint8_t g = random(80);
        setNeoPixelColor(i, Adafruit_NeoPixel::Color(r, g, 0));
    }
}

// Sparkle - random single LED flashes white
static void bling_sparkle() {
    for (int i = 0; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, 0);
    setNeoPixelColor(random(NUM_NEOPIXELS), Adafruit_NeoPixel::Color(255, 255, 255));
}

static void bling_off() {
    clearNeoPixels();
}

void bling_stop_animation() {
    bling_ticker.detach();
    bling_mode = 0;
    bling_off();
}

static const char* morse_ad_astra = ".- -..  .- ... - .-. .-"; // AD ASTRA
static int morse_pos = 0;
static int morse_sub = 0; // sub-tick within element

static inline uint32_t packColor(uint8_t r, uint8_t g, uint8_t b) {
    return Adafruit_NeoPixel::Color(r, g, b);
}

static void setAllPixels(uint32_t color) {
    for (int i = 0; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, color);
}

static void bling_morse() {
    char c = morse_ad_astra[morse_pos];
    if (c == '\0') { morse_pos = 0; clearNeoPixels(); return; }
    if (c == ' ') {
        clearNeoPixels();
        morse_sub++;
        if (morse_sub >= 3) { morse_sub = 0; morse_pos++; }
    } else if (c == '.') {
        if (morse_sub == 0) setAllPixels(packColor(0, 180, 255));
        else clearNeoPixels();
        morse_sub++;
        if (morse_sub >= 2) { morse_sub = 0; morse_pos++; }
    } else if (c == '-') {
        if (morse_sub < 3) setAllPixels(packColor(0, 180, 255));
        else clearNeoPixels();
        morse_sub++;
        if (morse_sub >= 4) { morse_sub = 0; morse_pos++; }
    }
}

static void start_bling_animation(int mode) {
    bling_mode = mode;
    bling_ticker.detach();
    morse_pos = 0; morse_sub = 0;
    if (mode == 0) {
        bling_off();
        return;
    }
    bling_ticker.attach_ms(200, bling_animate); // 200ms interval for all patterns
}

static void bling_animate() {
    switch (bling_mode) {
        case 1: bling_rainbow_cycle(); break;
        case 2: bling_police_lights(); break;
        case 3: bling_all_blink(); break;
        case 4: bling_chase(); break;
        case 5: bling_random(); break;
        case 6: bling_breathing(); break;
        case 7: bling_aurora(); break;
        case 8: bling_morse(); break;
        case 9: bling_comet(); break;
        case 10: bling_fire(); break;
        case 11: bling_sparkle(); break;
        default: bling_off(); break;
    }
}

// --- Bling window UI ---
void create_bling_window() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);
    bling_ticker.detach();
    bling_mode = 0;

    // HUD title
    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text(hdr, "LED ARRAY CONTROL");
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_unscii_8, 0);
    lv_obj_set_pos(hdr, 8, 8);

    // Accent line
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 312, 1);
    lv_obj_set_pos(line, 4, 22);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    static const struct { const char *name; int mode; } modes[] = {
        {"Rainbow",  1}, {"Police",  2}, {"Blink",   3}, {"Chase",   4},
        {"Random",   5}, {"Breathe", 6}, {"Aurora",   7}, {"Morse",   8},
        {"Comet",    9}, {"Fire",   10}, {"Sparkle", 11}, {"Off",     0},
    };

    for (int i = 0; i < 12; i++) {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 74, 48);
        lv_obj_set_pos(btn, 4 + (i % 4) * 78, 26 + (i / 4) * 52);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a2e), 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            start_bling_animation((int)(intptr_t)lv_event_get_user_data(e));
        }, LV_EVENT_CLICKED, (void*)(intptr_t)modes[i].mode);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, modes[i].name);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);
        lv_obj_center(lbl);
    }

    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x888888), 0);
    lv_obj_center(bl);
}
