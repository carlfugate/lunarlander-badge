#pragma once

#include <Adafruit_NeoPixel.h>

extern Adafruit_NeoPixel statusLED;

// Functions to manage status LED states
void initStatusLED();
void setSolidYellow();   // Solid yellow (scanning)
void setSolidBlue();     // Solid blue (networks found)
void setSolidRed();      // Solid red (no networks found)
void setSolidGreen();      // Solid green (connected to Wi-Fi)
void clearLED();         // Turn off LED

