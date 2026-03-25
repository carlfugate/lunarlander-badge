#pragma once

#include <WiFi.h>
#include <Arduino.h>
#include <FT6336U.h>
#include <lvgl.h>
#include "src/widgets/arc/lv_arc.h"
#include <Adafruit_MAX1704X.h>  // Include MAX17048 library
#include <SPI.h>
#include <SD.h>
#include <ESP32RotaryEncoder.h>
#include <set>
#include <TFT_eSPI.h>  // TFT library
#include <time.h>
#include "pins.h"
#include "Hardware/Status_LED.h"
#include "Hardware/WiFi_Module.h"     // Wi-Fi scanning functionality
#include "Hardware/NeoPixelControl.h"
#include "Hardware/Screen_Module.h"
#include "QA/WiFi_Settings.h"   // Wi-Fi connection settings
#include "QA/Diagnostics.h"
#include "QA/PirateShipAnimation.h"
#include "QA/ota.hpp"
#include "QA/Menu.h"
#include "Game/LunarState.h"
#include "QA/QA_Test_Sequence.h"
#include "QA/ntp_sync.h"


extern bool ledStatus;
extern int ledPins[];  // Declaration of the LED pins array (size inferred from the definition)
extern int numLeds;
extern uint32_t badge_boot_ms;