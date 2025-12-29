#include "QA/Bling.hpp"
#include "Includes.h"
#include "Hardware/NeoPixelControl.h"
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>

// Forward declaration for back button
extern void create_main_menu(bool show_ota_check);
extern "C" lv_style_t style_modern_btns[10];
extern lv_obj_t* create_basic_window();

static lv_obj_t* BlingWindow = nullptr;

static Ticker bling_ticker;
static int bling_mode = 0; // 0=none, 1=rainbow, 2=police, 3=all blink, 4=chase, 5=random

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

static void bling_off() {
    clearNeoPixels();
}

static void start_bling_animation(int mode) {
    bling_mode = mode;
    bling_ticker.detach();
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
        default: bling_off(); break;
    }
}

// --- Button event handler ---
static void bling_button_event_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* label = lv_label_get_text(lv_obj_get_child(btn, 0));
    if (strcmp(label, "Rainbow") == 0) start_bling_animation(1);
    else if (strcmp(label, "Police") == 0) start_bling_animation(2);
    else if (strcmp(label, "All Blink") == 0) start_bling_animation(3);
    else if (strcmp(label, "Chase") == 0) start_bling_animation(4);
    else if (strcmp(label, "Random") == 0) start_bling_animation(5);
    else if (strcmp(label, "Off") == 0) start_bling_animation(0);
}

// --- Bling window UI ---
void create_bling_window() {
    if (BlingWindow) {
        lv_obj_del(BlingWindow);
        BlingWindow = nullptr;
    }
    BlingWindow = create_basic_window();
    lv_scr_load(BlingWindow);
    bling_ticker.detach(); // Stop animation when opening window
    bling_mode = 0;

    // Add Bling buttons (restored from previous working version)
    const char* bling_modes[] = {"Rainbow", "Police", "All Blink", "Chase", "Random", "Off"};
    int num_modes = sizeof(bling_modes)/sizeof(bling_modes[0]);
    for (int i = 0; i < num_modes; ++i) {
        lv_obj_t* btn = lv_btn_create(BlingWindow);
        lv_obj_add_style(btn, &style_modern_btns[i%10], 0);
        lv_obj_set_size(btn, 100, 48);
        lv_obj_add_event_cb(btn, bling_button_event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, bling_modes[i]);
        lv_obj_center(label);
    }
    // Back button
    lv_obj_t* back_btn = lv_btn_create(BlingWindow);
    lv_obj_add_style(back_btn, &style_modern_btns[0], 0);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e){ create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
}
