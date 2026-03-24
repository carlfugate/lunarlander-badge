#include "Hardware/Screen_Module.h"
#include <lvgl.h>
#include "pins.h"
#include <Wire.h>
#include "FT6336U.h" // Ensure this is the correct path for your FT6336U library

TFT_eSPI tft = TFT_eSPI();  // Create TFT instance
// Use actual integer pin numbers for FT6336U constructor
static FT6336U ft6336u(8, 9, 3, 7);

void setScreenRotation(uint8_t rotation) {
    tft.setRotation(rotation);
    // Map LVGL rotation to TFT_eSPI (0-3)
    uint8_t tft_rotation = 0;
    switch(rotation) {
        case 0: // LV_DISPLAY_ROTATION_0
            tft_rotation = 0;
            break;
        case 1: // LV_DISPLAY_ROTATION_90
            tft_rotation = 1;
            break;
        case 2: // LV_DISPLAY_ROTATION_180
            tft_rotation = 2;
            break;
        case 3: // LV_DISPLAY_ROTATION_270
            tft_rotation = 3;
            break;
        default:
            tft_rotation = 0;
    }
    tft.setRotation(tft_rotation);
}

void bootColorScheme() {
    tft.fillScreen(TFT_BLACK);  // Set the background color to black
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN); // Set the text color to green
    tft.setCursor(tft.width() / 4 - 60, tft.height() / 2 - 10);  // Center the text
}

void displayInitializingMessage() {
    bootColorScheme();
    tft.println("Initializing Hardware...");
}

void displayInitializingWifiMessage() {
    bootColorScheme();
    tft.println("...Initializing Wifi...");
}

void displayCheckingForUpdatesMessage() {
    bootColorScheme();
    tft.println("..Checking For Updates..");
}

void displayWelcomeMessage() {
    bootColorScheme();
    tft.println("...Finalizing Menu...");
}

// This was used to show that touch was working, not currently used
void displayCoordinates(uint x, uint y) {
    bootColorScheme();
    tft.printf("X: %d, Y: %d\n", x, y);
}

void Screen_Module_InitTouch() {
    Wire.begin();
    ft6336u.begin();
}

bool Screen_Module_GetTouch(uint16_t &x, uint16_t &y) {
    if (ft6336u.read_td_status()) {
        // For 180 degree screen rotation, invert both axes
        x = TFT_HOR_RES - ft6336u.read_touch1_x();
        y = TFT_VER_RES - ft6336u.read_touch1_y();
        return true;
    }
    return false;
}

void displayBootStatusLine(const char* msg, bool success) {
    tft.setTextColor(success ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.println(msg);
}

void displayBootTerminalHeader() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    // "BSidesKC" = 8 chars * 12px = 96px, center on 320px
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(112, 10);
    tft.println("BSidesKC");
    // "2 0 2 6" = 7 chars * 12px = 84px
    tft.setTextColor(0x07FF, TFT_BLACK);
    tft.setCursor(118, 32);
    tft.println("2 0 2 6");
    tft.println("");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(1);
}

void displayBootComplete(bool allOk) {
    tft.println("");
    if (allOk) {
        tft.setTextColor(0x07FF, TFT_BLACK);  // Cyan
        tft.println("> All systems nominal.");
    } else {
        tft.setTextColor(0xFE60, TFT_BLACK);  // Amber/orange
        tft.println("> Partial systems. Offline mode.");
    }
    tft.println("");
    tft.setTextColor(0x528A, TFT_BLACK);
    // 28 chars * 6px = 168px at textSize 1, center on 320
    tft.setCursor(76, 220);
    tft.println("April 25, 2026 - Kansas City");
}

