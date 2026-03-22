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

// --- Buzzer ---

void audio_thrust_start() { tone(BUZZER_PIN, 200); }
void audio_thrust_stop()  { noTone(BUZZER_PIN); }

void audio_landed() {
    uint16_t notes[] = {440, 554, 659};
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, notes[i]);
        delay(150);
    }
    noTone(BUZZER_PIN);
}

void audio_crashed() {
    for (uint16_t f = 400; f >= 100; f -= 30) {
        tone(BUZZER_PIN, f);
        delay(50);
    }
    noTone(BUZZER_PIN);
}

void audio_low_fuel() {
    tone(BUZZER_PIN, 1000);
    delay(50);
    noTone(BUZZER_PIN);
}

// --- LEDs ---

void leds_thrust()  { setAllPixels(packColor(255, 100, 0)); }
void leds_landed()  { setAllPixels(packColor(0, 255, 0)); }
void leds_crashed() { setAllPixels(packColor(255, 0, 0)); }
void leds_idle()    { clearNeoPixels(); }

#else // NATIVE_TEST stubs

void audio_thrust_start() {}
void audio_thrust_stop()  {}
void audio_landed()       {}
void audio_crashed()      {}
void audio_low_fuel()     {}

void leds_thrust()  {}
void leds_landed()  {}
void leds_crashed() {}
void leds_idle()    {}

#endif
