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
    tone(BUZZER_PIN, 200);
}
void audio_thrust_stop()  { noTone(BUZZER_PIN); }

void audio_landed() {
    if (s_muted) return;
    uint16_t notes[] = {440, 554, 659};
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, notes[i]);
        delay(150);
    }
    noTone(BUZZER_PIN);
}

void audio_crashed() {
    if (s_muted) return;
    for (uint16_t f = 400; f >= 100; f -= 30) {
        tone(BUZZER_PIN, f);
        delay(50);
    }
    noTone(BUZZER_PIN);
}

void audio_low_fuel() {
    if (s_muted) return;
    tone(BUZZER_PIN, 1000);
    delay(50);
    noTone(BUZZER_PIN);
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

#else // NATIVE_TEST stubs

void audio_set_mute(bool) {}
bool audio_is_muted() { return false; }
void audio_click() {}
void audio_click_back() {}

void audio_thrust_start() {}
void audio_thrust_stop()  {}
void audio_landed()       {}
void audio_crashed()      {}
void audio_low_fuel()     {}

void leds_thrust()  {}
void leds_landed()  {}
void leds_crashed() {}
void leds_idle()    {}
void leds_fuel_gauge(float, float) {}

#endif
