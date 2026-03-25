#include "Game/LunarAudio.h"

#ifndef NATIVE_TEST

#include <Arduino.h>
#include "pins.h"
#include "Hardware/NeoPixelControl.h"

static inline uint32_t packColor(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void setAllPixels(uint32_t color) {
    for (uint16_t i = 0; i < NUM_NEOPIXELS; i++)
        setNeoPixelColor(i, color);
}

// --- Mute ---

static bool s_muted = false;

void audio_set_mute(bool muted) { s_muted = muted; }
bool audio_is_muted() { return s_muted; }

// --- Click sounds ---

void audio_click() {
    if (s_muted) return;
    tone(BUZZER_PIN, 2000, 30);
}

void audio_click_back() {
    if (s_muted) return;
    tone(BUZZER_PIN, 1500, 30);
}

// --- Buzzer ---

void audio_thrust_start() {
    if (s_muted) return;
    tone(BUZZER_PIN, 150);
}
void audio_thrust_stop()  { noTone(BUZZER_PIN); }

void audio_landed() {
    if (s_muted) return;
    uint16_t melody[] = {523, 659, 784, 1047, 1319}; // C5-E5-G5-C6-E6
    for (int i = 0; i < 5; i++) {
        tone(BUZZER_PIN, melody[i], 80);
        delay(100);
    }
    noTone(BUZZER_PIN);
}

void audio_crashed() {
    if (s_muted) return;
    for (uint16_t f = 800; f >= 100; f -= 50) {
        tone(BUZZER_PIN, f, 20);
        delay(20);
    }
    noTone(BUZZER_PIN);
}

void audio_low_fuel() {
    if (s_muted) return;
    tone(BUZZER_PIN, 1000);
    delay(50);
    noTone(BUZZER_PIN);
}

static uint32_t last_alt_beep = 0;

void audio_altitude_warning(float altitude) {
    if (s_muted) return;
    if (altitude > 80 || altitude < 0) return;
    uint32_t now = millis();
    uint32_t interval;
    if (altitude < 10) interval = 150;
    else if (altitude < 25) interval = 250;
    else if (altitude < 50) interval = 500;
    else interval = 1000;
    if (now - last_alt_beep >= interval) {
        tone(BUZZER_PIN, 1000, 40);
        last_alt_beep = now;
    }
}

// --- LEDs ---

void leds_thrust() {
    setNeoPixelColor(0, packColor(255, 140, 0));
    setNeoPixelColor(1, packColor(255, 200, 50));
}

void leds_fuel_gauge(float fuel, float max_fuel) {
    float pct = fuel / max_fuel;
    int lit = (int)(pct * NUM_NEOPIXELS + 0.5f);
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        if (i < lit) {
            if (pct < 0.1f) setNeoPixelColor(i, packColor(255, 0, 0));
            else if (pct < 0.2f) setNeoPixelColor(i, packColor(255, 170, 0));
            else setNeoPixelColor(i, packColor(0, 180, 0));
        } else {
            setNeoPixelColor(i, 0);
        }
    }
}

void leds_landed() {
    for (int flash = 0; flash < 3; flash++) {
        setAllPixels(packColor(0, 255, 0));
        delay(100);
        setAllPixels(0);
        delay(100);
    }
    setAllPixels(packColor(0, 80, 0));
}

void leds_crashed() {
    for (int flash = 0; flash < 5; flash++) {
        setAllPixels(packColor(255, 0, 0));
        delay(50);
        setAllPixels(0);
        delay(50);
    }
}

void leds_idle() { clearNeoPixels(); }

void audio_boot() {
    if (s_muted) return;
    uint16_t notes[] = {262, 330, 392, 523}; // C4-E4-G4-C5
    for (int i = 0; i < 4; i++) {
        tone(BUZZER_PIN, notes[i], 100);
        delay(120);
    }
    noTone(BUZZER_PIN);
}

void audio_countdown(int seconds_left) {
    if (s_muted) return;
    if (seconds_left <= 0) {
        tone(BUZZER_PIN, 2000, 500);
    } else if (seconds_left <= 3) {
        tone(BUZZER_PIN, 1500, 100);
    } else if (seconds_left <= 10) {
        tone(BUZZER_PIN, 1000, 80);
    }
}

void leds_team_color(uint8_t player_index) {
    static const uint32_t team_colors[] = {
        0x0000FF, 0xFF0000, 0x00FF00, 0xFF00FF, 0xFFFF00, 0x00FFFF
    };
    uint32_t c = team_colors[player_index % 6];
    uint8_t r = ((c >> 16) & 0xFF) / 7;
    uint8_t g = ((c >> 8) & 0xFF) / 7;
    uint8_t b = (c & 0xFF) / 7;
    setAllPixels(packColor(r, g, b));
}

void audio_achievement() {
    if (s_muted) return;
    tone(BUZZER_PIN, 784, 80); delay(100);
    tone(BUZZER_PIN, 1047, 80); delay(100);
    delay(80);
    tone(BUZZER_PIN, 1319, 300);
    delay(350);
    noTone(BUZZER_PIN);
}

void leds_achievement() {
    for (int i = 0; i < NUM_NEOPIXELS; i++) {
        setNeoPixelColor(i, Adafruit_NeoPixel::ColorHSV(i * 65536L / NUM_NEOPIXELS, 255, 200));
        delay(100);
    }
    delay(200);
    for (int i = 0; i < NUM_NEOPIXELS; i++)
        setNeoPixelColor(i, Adafruit_NeoPixel::Color(255, 180, 0));
    delay(1000);
    clearNeoPixels();
}

#else // NATIVE_TEST stubs

static bool s_muted = false;
void audio_set_mute(bool m) { s_muted = m; }
bool audio_is_muted() { return s_muted; }
void audio_click() {}
void audio_click_back() {}

void audio_thrust_start() {}
void audio_thrust_stop()  {}
void audio_landed()       {}
void audio_crashed()      {}
void audio_low_fuel()     {}
void audio_altitude_warning(float) {}

void leds_thrust()  {}
void leds_landed()  {}
void leds_crashed() {}
void leds_idle()    {}
void leds_fuel_gauge(float, float) {}
void audio_boot() {}
void audio_countdown(int) {}
void leds_team_color(uint8_t) {}
void audio_achievement() {}
void leds_achievement() {}

#endif
