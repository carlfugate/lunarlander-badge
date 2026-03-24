#pragma once

#include "Includes.h"   // Contains common definitions and includes
#include <lvgl.h> // Include the header file that defines lv_obj_t

#define TFT_HOR_RES   240
#define TFT_VER_RES   320
#define TFT_ROTATION  LV_DISPLAY_ROTATION_270

extern Adafruit_MAX17048 max17048; // Extern declaration for use in menu.cpp

// WiFi-related global variables
extern lv_obj_t * WifiWindow;
extern lv_obj_t * WifiScanWindow;
extern lv_obj_t * keyboard;
extern lv_obj_t * password_textarea;
extern String selected_ssid;

void create_main_menu(bool show_ota_check = true);
void load_screen_and_delete_old(lv_obj_t *new_scr);
extern lv_obj_t * main_menu;
// Declare other global variables if needed

// Function prototypes for creating windows/menus
void create_back_button(lv_obj_t * parent, lv_event_cb_t back_cb = nullptr);
void create_battery_window();
void create_buzzer_window();
void create_neo_window();
void create_sd_card_window();
void create_wifi_scan_window();
void create_wifi_window();
void create_wifi_scan_window();
void create_system_info_window();
void create_credits_window();
void create_checkin_window();
bool isVersionNewer(const String& online, const String& local);