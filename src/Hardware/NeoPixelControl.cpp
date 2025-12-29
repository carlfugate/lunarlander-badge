#include "Hardware/NeoPixelControl.h"
#include "pins.h"

// Create NeoPixel object for the LEDs on GPIO18
Adafruit_NeoPixel neoPixels = Adafruit_NeoPixel(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Initialize NeoPixels
void initNeoPixels() {
    neoPixels.begin();
    neoPixels.setBrightness(33); // Adjust brightness (0-255)
    neoPixels.show();            // Initialize all pixels to 'off'
}

// Set color of a specific NeoPixel (n is the LED index, 0-based)
void setNeoPixelColor(uint16_t n, uint32_t color) {
    if (n < NUM_NEOPIXELS) {
        neoPixels.setPixelColor(n, color);
        neoPixels.show();
    }
}

// Turn off all NeoPixels
void clearNeoPixels() {
    for (uint16_t i = 0; i < NUM_NEOPIXELS; i++) {
        neoPixels.setPixelColor(i, 0); // Set each pixel to 'off'
    }
    neoPixels.show();
}

// Function to cycle colors on the NeoPixels
void cycleNeoPixelColors() {
    static uint8_t colorIndex = 0;  // Tracks the current color index

    // Define color sequence (red, green, blue)
    uint32_t colors[] = {
        neoPixels.Color(255, 0, 0),   // Red
        neoPixels.Color(0, 255, 0),   // Green
        neoPixels.Color(0, 0, 255)    // Blue
    };
    const uint8_t numColors = sizeof(colors) / sizeof(colors[0]);

    // Set each NeoPixel to the current color in the sequence
    for (uint16_t i = 0; i < NUM_NEOPIXELS; i++) {
        neoPixels.setPixelColor(i, colors[colorIndex]);
    }
    neoPixels.show();

    // Move to the next color in the sequence for the next cycle
    colorIndex = (colorIndex + 1) % numColors;
}
