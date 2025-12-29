// Status_LED.cpp

#include "Hardware/Status_LED.h"
#include "pins.h"

// Define and initialize the NeoPixel for status indication
Adafruit_NeoPixel statusLED(NUM_STATUS_LEDS, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

void initStatusLED() {
    statusLED.begin();
    statusLED.setBrightness(50); // Adjust brightness as needed
    statusLED.show();
}

// Set LED to solid yellow
void setSolidYellow() {
    statusLED.setPixelColor(0, statusLED.Color(255, 255, 0)); // Yellow
    statusLED.show();
}

// Set LED to solid blue
void setSolidBlue() {
    statusLED.setPixelColor(0, statusLED.Color(0, 0, 255)); // Blue
    statusLED.show();
}

// Set LED to solid red
void setSolidRed() {
    statusLED.setPixelColor(0, statusLED.Color(255, 0, 0)); // Red
    statusLED.show();
}

// Set LED to solid green
void setSolidGreen() {
    statusLED.setPixelColor(0, statusLED.Color(0, 255, 0)); // Green
    statusLED.show();
}

// Clear LED (turn off)
void clearLED() {
    statusLED.clear();
    statusLED.show();
}
