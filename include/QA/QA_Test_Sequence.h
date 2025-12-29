#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Declare shared variables for LED colors and count
extern uint32_t colors[];
extern int numColors;
void printDeviceInfo();

// Run all hardware self-tests and report results
void runHardwareSelfTest();