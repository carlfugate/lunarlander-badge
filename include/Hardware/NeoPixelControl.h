#pragma once

#include <Adafruit_NeoPixel.h>

void initNeoPixels();
void setNeoPixelColor(uint16_t n, uint32_t color);
void clearNeoPixels();
void cycleNeoPixelColors();  // New function to cycle colors

