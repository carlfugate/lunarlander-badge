#pragma once
#include "Includes.h"

// Function declarations
void setScreenRotation(uint8_t rotation);
void displayInitializingMessage();
void displayInitializingWifiMessage();
void displayCheckingForUpdatesMessage();
void displayWelcomeMessage();
void displayCoordinates(uint x, uint y);
void Screen_Module_InitTouch();
bool Screen_Module_GetTouch(uint16_t &x, uint16_t &y);
void displayBootStatusLine(const char* msg, bool success = true);
void displayBootTerminalHeader();