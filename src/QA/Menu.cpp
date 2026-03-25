// menu.cpp
#include "QA/Menu.h"
#include "QA/ota.hpp"
#include "QA/schedule.h"
#include "QA/Bling.hpp"
#include "includes.h"   // Contains common definitions and includes
#include "Hardware/BadgeRegistration.h"
#include "Hardware/BadgeVersion.h"
#include "Game/LunarState.h"
#include "Game/LunarAudio.h"
#include "QA/Screensaver.h"

void create_checkin_window();
void create_credits_window();
static void create_system_submenu();

// Konami code easter egg: ↑↑↓↓←→←→ via touch screen edges
static const int konami_seq[] = {0,0,1,1,2,3,2,3}; // 0=up,1=down,2=left,3=right
static int konami_pos = 0;
static bool konami_unlocked = false;

static int touch_to_direction(int x, int y) {
    if (y < 40) return 0;
    if (y > 200) return 1;
    if (x < 60) return 2;
    if (x > 260) return 3;
    return -1;
}

static void konami_check(int dir) {
    if (dir < 0) return;
    if (dir == konami_seq[konami_pos]) {
        konami_pos++;
        if (konami_pos >= 8) {
            konami_pos = 0;
            konami_unlocked = true;
            for (int i = 0; i < 6; i++)
                setNeoPixelColor(i, Adafruit_NeoPixel::ColorHSV(i * 65536L / 6, 255, 255));
        }
    } else {
        konami_pos = (dir == konami_seq[0]) ? 1 : 0;
    }
}

// External variables
extern bool max17048_available;  // Defined in main.cpp

// Global variables (definitions)
lv_timer_t * ota_timer = nullptr;
lv_obj_t * main_menu       = nullptr;
lv_obj_t * NeoWindow       = nullptr;
lv_obj_t * BuzzerWindow    = nullptr;
lv_obj_t * BatteryWindow   = nullptr;
lv_obj_t * SDCardWindow    = nullptr;
lv_obj_t * SystemWindow    = nullptr;
lv_obj_t * CreditsWindow  = nullptr;
lv_obj_t * buzzer_slider_label = nullptr;
lv_obj_t * ScheduleWindow = nullptr;

bool performOtaUpdateCheck = true;

static void log_heap(const char *tag) {
    Serial.printf("[HEAP] %s: free=%d, min=%d\n", tag, ESP.getFreeHeap(), ESP.getMinFreeHeap());
}

void load_screen_and_delete_old(lv_obj_t *new_scr) {
    lv_obj_t *old = lv_scr_act();
    lv_scr_load(new_scr);
    if (old && old != new_scr) {
        lv_obj_del(old);
    }
}

// Modern button style with gradient and shadow, now supports custom colors
lv_style_t style_modern_btns[10];
static lv_color_t btn_grad_start[10] = {
    lv_color_hex(0x4e54c8), lv_color_hex(0xff512f), lv_color_hex(0x11998e),
    lv_color_hex(0xf7971e), lv_color_hex(0x43cea2), lv_color_hex(0x4776e6),
    lv_color_hex(0x614385), lv_color_hex(0x02aab0), lv_color_hex(0xf46b45), lv_color_hex(0x7b4397)
};
static lv_color_t btn_grad_end[10] = {
    lv_color_hex(0x8f94fb), lv_color_hex(0xdd2476), lv_color_hex(0x38ef7d),
    lv_color_hex(0xffe259), lv_color_hex(0x185a9d), lv_color_hex(0x8e54e9),
    lv_color_hex(0x516395), lv_color_hex(0x00cdac), lv_color_hex(0xeea849), lv_color_hex(0xdc2430)
};

void init_modern_button_styles() {
    for (int i = 0; i < 10; ++i) {
        lv_style_init(&style_modern_btns[i]);
        lv_style_set_bg_color(&style_modern_btns[i], btn_grad_start[i]);
        lv_style_set_bg_grad_color(&style_modern_btns[i], btn_grad_end[i]);
        lv_style_set_bg_grad_dir(&style_modern_btns[i], LV_GRAD_DIR_VER);
        lv_style_set_radius(&style_modern_btns[i], 16);
        lv_style_set_shadow_width(&style_modern_btns[i], 16);
        lv_style_set_shadow_color(&style_modern_btns[i], lv_color_hex(0x222222));
        lv_style_set_shadow_ofs_x(&style_modern_btns[i], 4);
        lv_style_set_shadow_ofs_y(&style_modern_btns[i], 8);
        lv_style_set_border_width(&style_modern_btns[i], 2);
        lv_style_set_border_color(&style_modern_btns[i], btn_grad_start[i]);
        lv_style_set_text_color(&style_modern_btns[i], lv_color_hex(0xffffff));
    }
}

// Helper to create a modern button with a specific style index
lv_obj_t* create_modern_button(lv_obj_t* parent, const char* label_text, lv_event_cb_t event_cb, int style_idx) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &style_modern_btns[style_idx], 0);
    lv_obj_set_size(btn, 90, 48); // Smaller size for 3 per row
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, label_text);
    lv_obj_center(label); // Center horizontally and vertically
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_align(label, LV_ALIGN_CENTER, 0);
    if (event_cb) {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    }
    return btn;
}

// Helper to create a dark-themed window
lv_obj_t* create_basic_window() {
    lv_obj_t* win = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(win, lv_color_hex(0x0a0a0f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(win, LV_OPA_COVER, 0);
    return win;
}

lv_obj_t * create_styled_label(lv_obj_t * parent, const char * text, lv_align_t align, int x_offset, int y_offset) {
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), LV_PART_MAIN);
    lv_obj_align(label, align, x_offset, y_offset);
    return label;
}

//----------------------------------------------------
// Add OTA update check during boot
//----------------------------------------------------
void checkForOTAUpdate() {
    Serial.println("Checking for OTA updates...");

    if (WiFi.status() == WL_CONNECTED) {
        OTA ota;
        if (ota.isUpdateAvailable()) {
            Serial.println("Update available!");
            ota.showUpdateScreen();
        } else {
            Serial.println("No updates found. Proceeding to menu.");
            create_main_menu(false);
        }
    } else {
        Serial.println("No Wi-Fi connection. Skipping OTA update check and proceeding to menu.");
        create_main_menu(false);
    }
}

//----------------------------------------------------
// Create a back button that returns to the main menu
//----------------------------------------------------
static void back_to_system_cb(lv_event_t *e) { create_system_submenu(); }

void create_back_button(lv_obj_t * parent, lv_event_cb_t back_cb) {
    lv_obj_t * back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 80, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back_btn, back_cb ? back_cb : [](lv_event_t *e) { create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(back_label, lv_color_hex(0x888888), 0);
    lv_obj_center(back_label);
}

//----------------------------------------------------
// Create OTA Window
//----------------------------------------------------
void create_ota_window() {
    lv_obj_t *ota_window = create_basic_window();
    load_screen_and_delete_old(ota_window);

    // Get current version
    String codeVersion = BADGE_VERSION;

    // Get online version as string
    OTA ota;
    int availableVersion = ota.getAvailableVersion();
    String onlineVersion = availableVersion > 0 ? String(availableVersion / 100) + "." + String((availableVersion / 10) % 10) + "." + String(availableVersion % 10) : "?";
    String localVersion = BADGE_VERSION;

    char buf[128];
    snprintf(buf, sizeof(buf), "Current Version: %s\nOnline Version: %s", localVersion.c_str(), onlineVersion.c_str());
    lv_obj_t *label = lv_label_create(ota_window);
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    if (isVersionNewer(onlineVersion, localVersion)) {
        lv_obj_t *update_btn = lv_btn_create(ota_window);
        lv_obj_set_size(update_btn, 140, 50);
        lv_obj_align(update_btn, LV_ALIGN_CENTER, 0, 30);
        lv_obj_t *update_label = lv_label_create(update_btn);
        lv_label_set_text(update_label, "Update Now");
        lv_obj_center(update_label);
        lv_obj_add_event_cb(update_btn, [](lv_event_t * e) {
            // Show progress label
            const lv_obj_t *target = (const lv_obj_t *)lv_event_get_target(e);
            lv_obj_t * parent = lv_obj_get_parent(target);
            lv_obj_t *progress = lv_label_create(parent);
            lv_label_set_text(progress, "Starting OTA update...\nDo not power off.");
            lv_obj_align(progress, LV_ALIGN_CENTER, 0, 60);
            lv_refr_now(NULL);
            // Start OTA update (blocking)
            OTA::checkOTASync();
        }, LV_EVENT_CLICKED, NULL);
    } else if (availableVersion == -1) {
        lv_obj_t *err_label = lv_label_create(ota_window);
        lv_label_set_text(err_label, "Could not check online version.");
        lv_obj_set_style_text_color(err_label, lv_color_hex(0xff4444), LV_PART_MAIN);
        lv_obj_align(err_label, LV_ALIGN_CENTER, 0, 30);
    } else {
        lv_obj_t *up_to_date = lv_label_create(ota_window);
        lv_label_set_text(up_to_date, "You are running the latest version.");
        lv_obj_set_style_text_color(up_to_date, lv_color_hex(0x00c853), LV_PART_MAIN);
        lv_obj_align(up_to_date, LV_ALIGN_CENTER, 0, 30);
    }
    create_back_button(ota_window, back_to_system_cb);
}

//----------------------------------------------------
// Create Battery Window
//----------------------------------------------------
void create_battery_window() {
    lv_obj_t *BatteryWindow = create_basic_window();
    load_screen_and_delete_old(BatteryWindow);

    char buf[100];
    float bat_cent = 0.0;
    float bat_volt = 0.0;
    
    // Check if MAX17048 is available before reading
    if (max17048_available) {
        bat_cent = max17048.cellPercent();
        bat_volt = max17048.cellVoltage();
        snprintf(buf, sizeof(buf), "Battery: %.2f%%\nVoltage: %.2fV", bat_cent, bat_volt);
    } else {
        snprintf(buf, sizeof(buf), "Battery Monitor\nNot Available\n\nCheck Hardware");
    }

    create_styled_label(BatteryWindow, buf, LV_ALIGN_TOP_MID, 0, 10);

    // Only create the battery scale if MAX17048 is available
    if (max17048_available) {
        lv_obj_t *scale = lv_scale_create(BatteryWindow);
        lv_scale_set_range(scale, 0, 100);
        lv_scale_set_mode(scale, LV_SCALE_MODE_HORIZONTAL_TOP);
        lv_obj_set_size(scale, 220, 50);
        lv_obj_align(scale, LV_ALIGN_CENTER, 0, 40);
        lv_scale_set_total_tick_count(scale, 21);
        lv_scale_set_major_tick_every(scale, 5);
        lv_scale_set_label_show(scale, true);

        lv_color_t scale_color;
        if (bat_cent > 75) {
            scale_color = lv_color_hex(0x00FF00);
        } else if (bat_cent >= 25) {
            scale_color = lv_color_hex(0xFFFF00);
        } else {
            scale_color = lv_color_hex(0xFF0000);
        }
        lv_obj_set_style_line_color(scale, scale_color, LV_PART_MAIN);
    }

    create_back_button(BatteryWindow, back_to_system_cb);
}

//----------------------------------------------------
// Slider event callback for the buzzer window
//----------------------------------------------------
static void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
    int tone_value = lv_slider_get_value(slider);

    char buf[16];
    snprintf(buf, sizeof(buf), "Tone: %dHz", tone_value);
    lv_label_set_text(buzzer_slider_label, buf);

    if (tone_value == 0)
        noTone(BUZZER_PIN);
    else
        tone(BUZZER_PIN, tone_value);
}

//----------------------------------------------------
// Create Buzzer Window
//----------------------------------------------------
void create_buzzer_window() {
    BuzzerWindow = create_basic_window();
    load_screen_and_delete_old(BuzzerWindow);
    
    // Initialize LEDC for buzzer by briefly setting up a tone channel
    // This prevents the "LEDC is not initialized" error
    tone(BUZZER_PIN, 1000);
    delay(1);
    noTone(BUZZER_PIN);
    // (codeVersion not needed here, remove line)
    lv_obj_t * slider = lv_slider_create(BuzzerWindow);
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 0, 2000);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Create a label to display the slider value
    buzzer_slider_label = lv_label_create(BuzzerWindow);
    lv_label_set_text(buzzer_slider_label, "Tone: 0 Hz");
    lv_obj_align_to(buzzer_slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Add custom back button that stops buzzer before returning
    lv_obj_t * back_btn = lv_btn_create(BuzzerWindow);
    lv_obj_set_size(back_btn, 80, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        noTone(BUZZER_PIN);
        buzzer_slider_label = nullptr;
        BuzzerWindow = nullptr;
        create_system_submenu();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(back_label, lv_color_hex(0x888888), 0);
    lv_obj_center(back_label);
}

//----------------------------------------------------
// Event handler for NeoPixel button presses
//----------------------------------------------------
void neopixel_event_handler(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    const char * label = lv_label_get_text(lv_obj_get_child(btn, 0));

    if(strcmp(label, "Red") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(255, 0, 0));
        }
    } else if(strcmp(label, "Green") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(0, 255, 0));
        }
    } else if(strcmp(label, "Blue") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(0, 0, 255));
        }
    } else if(strcmp(label, "Off") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            clearNeoPixels();
        }
    }
}

//----------------------------------------------------
// Create NeoPixel Window
//----------------------------------------------------
void create_neo_window(void) {
    NeoWindow = create_basic_window();
    load_screen_and_delete_old(NeoWindow);

    const String buttons[] = {"Red", "Green", "Blue", "Off"};
    for (String name : buttons) {
        lv_obj_t * btn = lv_btn_create(NeoWindow);
        lv_obj_set_size(btn, 120, 50);
        lv_obj_add_event_cb(btn, neopixel_event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, name.c_str());
        lv_obj_center(label);
    }

    create_back_button(NeoWindow, back_to_system_cb);
}

//----------------------------------------------------
//----------------------------------------------------
void create_sd_card_window() {
    SDCardWindow = create_basic_window();
    load_screen_and_delete_old(SDCardWindow);

    File root = SD.open("/");
    if (!root) {
        Serial.println("Failed to open directory");
        lv_obj_t * error_label = lv_label_create(SDCardWindow);
        lv_label_set_text(error_label, "Failed to open SD card");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xff4444), LV_PART_MAIN);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        create_back_button(SDCardWindow, back_to_system_cb);
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        root.close();
        lv_obj_t * error_label = lv_label_create(SDCardWindow);
        lv_label_set_text(error_label, "Root is not a directory");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xff4444), LV_PART_MAIN);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        create_back_button(SDCardWindow, back_to_system_cb);
        return;
    }

    File file = root.openNextFile();
    while (file) {
        char buf[100];
        if (file.isDirectory()) {
            snprintf(buf, sizeof(buf), "DIR: %s", file.name());
        } else {
            snprintf(buf, sizeof(buf), "FILE: %s (%d bytes)", file.name(), file.size());
        }

        lv_obj_t * label = lv_label_create(SDCardWindow);
        lv_label_set_text(label, buf);
        lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        file = root.openNextFile();
    }
    
    root.close();
    create_back_button(SDCardWindow, back_to_system_cb);
}

//----------------------------------------------------
// Forward declaration for credits window
void create_credits_window();

//----------------------------------------------------
// Create Credits Window
//----------------------------------------------------
void create_credits_window(){
    log_heap("enter create_credits_window");
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);

    lv_obj_t *t1 = lv_label_create(scr);
    lv_label_set_text(t1, "BSidesKC");
    lv_obj_set_style_text_color(t1, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(t1, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *t2 = lv_label_create(scr);
    lv_label_set_text(t2, "2 0 2 6");
    lv_obj_set_style_text_color(t2, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(t2, LV_ALIGN_TOP_MID, 0, 26);

    lv_obj_t *t3 = lv_label_create(scr);
    lv_label_set_text(t3, "April 25 - Kansas City");
    lv_obj_set_style_text_color(t3, lv_color_hex(0x888888), 0);
    lv_obj_align(t3, LV_ALIGN_TOP_MID, 0, 44);

    lv_obj_t *body = lv_label_create(scr);
    lv_label_set_text(body, "Badge Hardware\nBadgePirates - bplabs.tech\n\n"
                            "Lunar Lander Game\nAd Astra Protocol\n\n"
                            "Powered by\nLVGL - ESP32-S3");
    lv_obj_set_style_text_color(body, lv_color_hex(0x00c853), 0);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 62);

    lv_obj_t *tag = lv_label_create(scr);
    lv_label_set_text(tag, "#BadgeLife");
    lv_obj_set_style_text_color(tag, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(tag, LV_ALIGN_BOTTOM_MID, 0, -38);

    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x888888), 0);
    lv_obj_center(bl);
}

//----------------------------------------------------
// Create System Info Window
//----------------------------------------------------
void create_system_info_window() {
    SystemWindow = create_basic_window();
    load_screen_and_delete_old(SystemWindow);

    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t usedHeap = totalHeap - freeHeap;
    float memoryUsagePercent = (float(usedHeap) / float(totalHeap)) * 100;

    String codeVersion = BADGE_VERSION;
    String codeName = BADGE_CODE_NAME;
    String macAddress = WiFi.macAddress();
    bool registered = isBadgeRegistered();
    String registrationTime = getRegistrationTime();
    String lastCheckin = getLastCheckinTime();
    String wifiSSID = getWiFiSSID();
    String registrationStatus = registered ? "Yes" : "No";
    if (registered && registrationTime.length() > 0) {
        registrationStatus += " at " + registrationTime;
    }

    char buf[700];
    snprintf(buf, sizeof(buf),
        "Code Version: %s\nCode: %s\nMAC Address: %s\nRegistered: %s\nLast Check-in: %s\nWiFi SSID: %s\n\nTotal Heap: %d bytes\nFree Heap: %d bytes\nUsed Heap: %d bytes (%.2f%% used)",
        codeVersion.c_str(), codeName.c_str(), macAddress.c_str(), registrationStatus.c_str(), lastCheckin.c_str(), wifiSSID.c_str(),
        totalHeap, freeHeap, usedHeap, memoryUsagePercent);

    lv_obj_t * label = lv_label_create(SystemWindow);
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10); // Align to top-left for better readability

    create_back_button(SystemWindow, back_to_system_cb);
}

//----------------------------------------------------
// Button event handler for the main menu
//----------------------------------------------------
void button_event_handler(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    const char * label = lv_label_get_text(lv_obj_get_child(btn, 0));

    if (strcmp(label, "RGBs") == 0) {
        create_neo_window();
    } else if (strcmp(label, "LEDs") == 0) {
        ledStatus = !ledStatus;
        for (int i = 0; i < numLeds; i++) {
            digitalWrite(ledPins[i], ledStatus);
        }
    } else if (strcmp(label, "Buzzer") == 0) {
        create_buzzer_window();
    } else if (strcmp(label, "OTA") == 0) {
        create_ota_window();
    } else if (strcmp(label, "Battery") == 0) {
        create_battery_window();
    } else if (strcmp(label, "Wifi") == 0) {
        create_wifi_window();
    } else if (strcmp(label, "SD Card") == 0) {
        create_sd_card_window();
    } else if (strcmp(label, "System") == 0) {
        create_system_info_window();
    } else if (strcmp(label, "Credits") == 0) {
        create_credits_window();
    } else if (strcmp(label, "Schedule") == 0) {
        loadSchedule();    // Load from SD or API
        displaySchedule(); // Show schedule using LVGL
    } else if (strcmp(label, "Bling") == 0) {
        create_bling_window();
    } else if (strcmp(label, "Check-In") == 0) {
        create_checkin_window();
    } else if (strcmp(label, "Lander") == 0) {
        lunar_lander_start();
    }
}

// Comms chatter ticker message pool
static const char* ticker_msgs[] = {
    "CAPCOM: Go for TLI",
    "FIDO: Trajectory nominal",
    "EECOM: Cryo pressure stable",
    "FLIGHT: We are GO",
    "RETRO: Entry corridor confirmed",
    "GNC: Guidance is GO",
    "TELMU: LM systems nominal",
    "CAPCOM: You are GO for landing",
    "SURGEON: Crew vitals nominal",
    "BOOSTER: All engines nominal",
    "NETWORK: Tracking stations acquired",
    "INCO: Telemetry stream active",
    "FAO: Flight activities on schedule",
    "CAPCOM: Eagle, you are GO for PDI",
    "EAGLE: The Eagle has landed",
    "HOUSTON: Roger, Tranquility",
    "CAPCOM: Beautiful, just beautiful",
    "FLIGHT: All stations, stand by",
    "EECOM: Fuel cells looking good",
    "GNC: AGC is GO",
    "RETRO: Go for orbit",
    "FIDO: Perigee 60.9 nautical miles",
    "CAPCOM: 10 seconds to ignition",
    "BOOSTER: Thrust is GO",
    "FLIGHT: Mark, T-minus 10",
    "SURGEON: Heart rates nominal",
    "NETWORK: Goldstone has AOS",
    "INCO: Data rate high",
    "CAPCOM: Godspeed",
    "FLIGHT: Resume countdown",
    "GNC: Pitch program complete",
    "EECOM: O2 flow nominal",
    "RETRO: Deorbit burn confirmed",
    "FIDO: Apogee 168 nautical miles",
    "TELMU: Descent stage armed",
    "CAPCOM: Ad astra per aspera",
    "FLIGHT: Kansas City, we are GO",
    "CAPCOM: BSidesKC crew, welcome aboard",
    "NETWORK: Badge mesh active",
    "INCO: Leaderboard synced",
    "FLIGHT: Contest window open",
    "CAPCOM: All crews report to stations",
    "SURGEON: Coffee levels critical",
    "EECOM: Snack reserves depleted",
    "FIDO: Hallway track trajectory set",
    "FLIGHT: CTF operations underway",
    "RETRO: After-party vector locked",
    "CAPCOM: Remember your callsign",
    "NETWORK: 6 NeoPixels standing by",
    "FLIGHT: Ad Astra Protocol active",
};
static const int NUM_TICKER_MSGS = sizeof(ticker_msgs) / sizeof(ticker_msgs[0]);

static void build_ticker_text(char *buf, size_t buflen) {
    int indices[8];
    for (int i = 0; i < 8; i++) {
        int idx;
        bool dup;
        do {
            idx = random(NUM_TICKER_MSGS);
            dup = false;
            for (int j = 0; j < i; j++) if (indices[j] == idx) { dup = true; break; }
        } while (dup);
        indices[i] = idx;
    }
    buf[0] = '\0';
    for (int i = 0; i < 8; i++) {
        if (i > 0) strlcat(buf, "    ", buflen);
        strlcat(buf, ticker_msgs[indices[i]], buflen);
    }
}

//----------------------------------------------------
// Display Main Menu Buttons
//----------------------------------------------------
void display_main_menu_buttons() {
    // Disable flex — use absolute positioning
    lv_obj_remove_style_all(main_menu);
    lv_obj_set_style_bg_color(main_menu, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_bg_opa(main_menu, LV_OPA_COVER, 0);
    lv_obj_clear_flag(main_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(main_menu, 0, 0);

    // Header bar with cyan accent line
    lv_obj_t *hdr = lv_obj_create(main_menu);
    lv_obj_set_size(hdr, 320, 26);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = lv_label_create(hdr);
    lv_label_set_text(brand, "BSidesKC '26");
    lv_obj_set_style_text_color(brand, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 8, 0);

    // Battery or USB indicator
    lv_obj_t *batt = lv_label_create(hdr);
    if (max17048_available) {
        float pct = max17048.cellPercent();
        if (pct > 100) pct = 100;
        lv_label_set_text_fmt(batt, LV_SYMBOL_BATTERY_FULL " %d%%", (int)pct);
    } else {
        lv_label_set_text(batt, LV_SYMBOL_USB " USB");
    }
    lv_obj_set_style_text_color(batt, lv_color_hex(0x888888), 0);
    lv_obj_align(batt, LV_ALIGN_RIGHT_MID, -8, 0);

    // MET clock
    static lv_obj_t *met_label = NULL;
    met_label = lv_label_create(hdr);
    lv_obj_set_style_text_color(met_label, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(met_label, &lv_font_montserrat_14, 0);
    lv_obj_align(met_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(met_label, "MET 00:00:00");

    static lv_timer_t *met_timer = NULL;
    if (met_timer) lv_timer_del(met_timer);
    met_timer = lv_timer_create([](lv_timer_t *t) {
        if (!met_label) return;
        uint32_t elapsed = (millis() - badge_boot_ms) / 1000;
        uint32_t h = elapsed / 3600;
        uint32_t m = (elapsed % 3600) / 60;
        uint32_t s = elapsed % 60;

        // Easter egg: 42 (4h 2m 0s = 14520s)
        static bool answer_shown = false;
        if (elapsed == 14520 && !answer_shown) {
            answer_shown = true;
            lv_label_set_text(met_label, "The answer is 42");
            for (int i = 0; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, 0xFFFFFF);
        } else if (answer_shown && elapsed > 14520 && elapsed <= 14525) {
            // Hold easter egg text for 5 seconds
        } else if (elapsed > 28800) { // 8 hours = party time
            static uint8_t party_hue = 0;
            party_hue += 3;
            lv_obj_set_style_text_color(met_label,
                lv_color_hsv_to_rgb(party_hue, 100, 100), 0);
            lv_label_set_text(met_label, LV_SYMBOL_AUDIO " PARTY");
        } else {
            if (answer_shown && elapsed > 14525) answer_shown = false;
            lv_label_set_text_fmt(met_label, "MET %02lu:%02lu:%02lu", h, m, s);
        }
    }, 1000, NULL);

    // Featured buttons: Lander + Schedule side by side
    lv_obj_t *lander_btn = lv_btn_create(main_menu);
    lv_obj_set_size(lander_btn, 148, 48);
    lv_obj_set_pos(lander_btn, 8, 32);
    lv_obj_set_style_bg_color(lander_btn, lv_color_hex(0x2e7d32), 0);
    lv_obj_set_style_bg_grad_color(lander_btn, lv_color_hex(0x00c853), 0);
    lv_obj_set_style_bg_grad_dir(lander_btn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(lander_btn, 10, 0);
    lv_obj_set_style_shadow_width(lander_btn, 8, 0);
    lv_obj_set_style_shadow_color(lander_btn, lv_color_hex(0x00c853), 0);
    lv_obj_set_style_shadow_opa(lander_btn, LV_OPA_30, 0);
    lv_obj_add_event_cb(lander_btn, [](lv_event_t *e) { lunar_lander_start(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ll = lv_label_create(lander_btn);
    lv_label_set_text(ll, LV_SYMBOL_PLAY " LANDER");
    lv_obj_center(ll);

    lv_obj_t *sched_btn = lv_btn_create(main_menu);
    lv_obj_set_size(sched_btn, 148, 48);
    lv_obj_set_pos(sched_btn, 164, 32);
    lv_obj_set_style_bg_color(sched_btn, lv_color_hex(0x0277bd), 0);
    lv_obj_set_style_bg_grad_color(sched_btn, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_bg_grad_dir(sched_btn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(sched_btn, 10, 0);
    lv_obj_set_style_shadow_width(sched_btn, 8, 0);
    lv_obj_set_style_shadow_color(sched_btn, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_shadow_opa(sched_btn, LV_OPA_30, 0);
    lv_obj_add_event_cb(sched_btn, [](lv_event_t *e) {
        loadSchedule();
        displaySchedule();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(sched_btn);
    lv_label_set_text(sl, LV_SYMBOL_LIST " SCHEDULE");
    lv_obj_center(sl);

    // Secondary buttons: 3x2 grid
    struct { const char *label; lv_color_t bg; lv_color_t grad; lv_event_cb_t cb; } items[] = {
        {LV_SYMBOL_OK " Check-In", lv_color_hex(0x4e54c8), lv_color_hex(0x8f94fb), [](lv_event_t *e) { create_checkin_window(); }},
        {LV_SYMBOL_TINT " Bling",   lv_color_hex(0x614385), lv_color_hex(0x516395), [](lv_event_t *e) { create_bling_window(); }},
        {LV_SYMBOL_WIFI " WiFi",    lv_color_hex(0x11998e), lv_color_hex(0x38ef7d), [](lv_event_t *e) { create_wifi_window(); }},
        {LV_SYMBOL_FILE " Credits", lv_color_hex(0xf7971e), lv_color_hex(0xffe259), [](lv_event_t *e) { create_credits_window(); }},
        {LV_SYMBOL_EYE_OPEN " CTF", lv_color_hex(0xff512f), lv_color_hex(0xdd2476), nullptr},
        {LV_SYMBOL_SETTINGS,         lv_color_hex(0x333333), lv_color_hex(0x555555), [](lv_event_t *e) { create_system_submenu(); }},
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_btn_create(main_menu);
        int col = i % 3;
        int row = i / 3;
        lv_obj_set_size(btn, 98, 38);
        lv_obj_set_pos(btn, 8 + col * 104, 88 + row * 44);
        lv_obj_set_style_bg_color(btn, items[i].bg, 0);
        lv_obj_set_style_bg_grad_color(btn, items[i].grad, 0);
        lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        if (items[i].cb) {
            lv_obj_add_event_cb(btn, items[i].cb, LV_EVENT_CLICKED, NULL);
        } else {
            lv_obj_add_state(btn, LV_STATE_DISABLED);
            lv_obj_set_style_opa(btn, LV_OPA_50, LV_STATE_DISABLED);
        }
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, items[i].label);
        lv_obj_center(lbl);
        // Dark text for yellow Credits button
        if (i == 3) lv_obj_set_style_text_color(lbl, lv_color_hex(0x333333), 0);
    }

    // Mission control label
    lv_obj_t *mc_label = lv_label_create(main_menu);
    lv_label_set_text(mc_label, LV_SYMBOL_GPS " MISSION CONTROL");
    lv_obj_set_style_text_color(mc_label, lv_color_hex(0x444455), 0);
    lv_obj_set_pos(mc_label, 8, 168);

    // Mission control ticker
    lv_obj_t *ticker_bg = lv_obj_create(main_menu);
    lv_obj_set_size(ticker_bg, 304, 22);
    lv_obj_set_pos(ticker_bg, 8, 180);
    lv_obj_set_style_bg_color(ticker_bg, lv_color_hex(0x111118), 0);
    lv_obj_set_style_border_width(ticker_bg, 1, 0);
    lv_obj_set_style_border_color(ticker_bg, lv_color_hex(0x222233), 0);
    lv_obj_set_style_radius(ticker_bg, 4, 0);
    lv_obj_set_style_pad_all(ticker_bg, 0, 0);
    lv_obj_clear_flag(ticker_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ticker = lv_label_create(ticker_bg);
    lv_label_set_long_mode(ticker, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(ticker, 290);
    static char ticker_buf[512];
    build_ticker_text(ticker_buf, sizeof(ticker_buf));
    lv_label_set_text(ticker, ticker_buf);
    lv_obj_set_style_text_color(ticker, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_anim_duration(ticker, lv_anim_speed(30), 0);
    lv_obj_align(ticker, LV_ALIGN_LEFT_MID, 4, 0);

    // Version in bottom-right corner
    lv_obj_t *ver = lv_label_create(main_menu);
    lv_label_set_text(ver, "v" BADGE_VERSION);
    lv_obj_set_style_text_color(ver, lv_color_hex(0x333333), 0);
    lv_obj_align(ver, LV_ALIGN_BOTTOM_RIGHT, -8, -4);

    // Konami code touch detector
    lv_obj_add_flag(main_menu, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(main_menu, [](lv_event_t *e) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_active(), &p);
        konami_check(touch_to_direction(p.x, p.y));
    }, LV_EVENT_PRESSED, NULL);
}

static void sys_btn_cb(lv_event_t *e) {
    const char *action = (const char *)lv_event_get_user_data(e);
    if (strcmp(action, "rgbs") == 0) create_neo_window();
    else if (strcmp(action, "leds") == 0) {
        ledStatus = !ledStatus;
        for (int i = 0; i < numLeds; i++) digitalWrite(ledPins[i], ledStatus);
    }
    else if (strcmp(action, "buzzer") == 0) create_buzzer_window();
    else if (strcmp(action, "sd") == 0) create_sd_card_window();
    else if (strcmp(action, "battery") == 0) create_battery_window();
    else if (strcmp(action, "ota") == 0) create_ota_window();
    else if (strcmp(action, "system") == 0) create_system_info_window();
}

static void create_system_submenu() {
    log_heap("enter create_system_submenu");
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);

    // Header
    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text(hdr, LV_SYMBOL_SETTINGS " System");
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 8, 6);

    // List items
    static const char *labels[] = {"RGBs", "LEDs", "Buzzer", "SD Card", "Battery", "OTA Update", "System Info"};
    static const char *actions[] = {"rgbs", "leds", "buzzer", "sd", "battery", "ota", "system"};
    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 304, 24);
        lv_obj_set_pos(btn, 8, 28 + i * 26);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0a0a0f), 0);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x222222), 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_event_cb(btn, sys_btn_cb, LV_EVENT_CLICKED, (void *)actions[i]);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_t *arrow = lv_label_create(btn);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(arrow, lv_color_hex(0x555555), 0);
        lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -4, 0);
    }

    // Sound toggle button
    lv_obj_t *snd_btn = lv_btn_create(scr);
    lv_obj_set_size(snd_btn, 100, 36);
    lv_obj_set_pos(snd_btn, 8, 28 + 7 * 26);
    lv_obj_set_style_bg_color(snd_btn, lv_color_hex(0x333333), 0);
    lv_obj_add_event_cb(snd_btn, [](lv_event_t *e) {
        audio_set_mute(!audio_is_muted());
        lv_obj_t *lbl = lv_obj_get_child((lv_obj_t*)lv_event_get_target(e), 0);
        lv_label_set_text(lbl, audio_is_muted() ? LV_SYMBOL_MUTE " MUTED" : LV_SYMBOL_AUDIO " SOUND");
        audio_click();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *snd_lbl = lv_label_create(snd_btn);
    lv_label_set_text(snd_lbl, audio_is_muted() ? LV_SYMBOL_MUTE " MUTED" : LV_SYMBOL_AUDIO " SOUND");
    lv_obj_center(snd_lbl);

    // Back button
    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x888888), 0);
    lv_obj_center(bl);
}

//----------------------------------------------------
// Create the Main Menu
//----------------------------------------------------
void create_main_menu(bool show_ota_check) {
    log_heap("enter create_main_menu");
    init_modern_button_styles();
    main_menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_menu, lv_color_hex(0x0a0a0f), LV_PART_MAIN);
    load_screen_and_delete_old(main_menu);

    if (show_ota_check && WiFi.status() == WL_CONNECTED) {
        OTA ota;
        int availableVersion = ota.getAvailableVersion();
        String onlineVersion = availableVersion > 0 ? String(availableVersion / 100) + "." + String((availableVersion / 10) % 10) + "." + String(availableVersion % 10) : "?";
        String localVersion = BADGE_VERSION;
        if (isVersionNewer(onlineVersion, localVersion)) {
            lv_obj_t *lbl = lv_label_create(main_menu);
            char buf[100];
            snprintf(buf, sizeof(buf), "Update: %s -> %s\nInstalling in 5s...", BADGE_VERSION, onlineVersion.c_str());
            lv_label_set_text(lbl, buf);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x00e5ff), 0);
            lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -20);

            lv_obj_t *skip = lv_btn_create(main_menu);
            lv_obj_set_size(skip, 100, 36);
            lv_obj_align(skip, LV_ALIGN_CENTER, 0, 30);
            lv_obj_set_style_bg_color(skip, lv_color_hex(0x333333), 0);
            lv_obj_add_event_cb(skip, [](lv_event_t *e) {
                if (ota_timer) { lv_timer_del(ota_timer); ota_timer = nullptr; }
                lv_obj_clean(main_menu);
                display_main_menu_buttons();
                screensaver_start_timer();
                // Heartbeat LED - double-pulse on LED 0
                static lv_timer_t *hb_timer = NULL;
                if (hb_timer) lv_timer_del(hb_timer);
                hb_timer = lv_timer_create([](lv_timer_t *t) {
                    static uint8_t hb_phase = 0;
                    hb_phase = (hb_phase + 1) % 20;
                    uint8_t brightness = 0;
                    if (hb_phase == 0 || hb_phase == 3) brightness = 30;
                    if (hb_phase == 1 || hb_phase == 4) brightness = 15;
                    setNeoPixelColor(0, Adafruit_NeoPixel::Color(0, brightness, brightness));
                    for (int i = 1; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, 0);
                }, 100, NULL);
            }, LV_EVENT_CLICKED, NULL);
            lv_obj_t *sl = lv_label_create(skip);
            lv_label_set_text(sl, "Skip");
            lv_obj_center(sl);

            ota_timer = lv_timer_create([](lv_timer_t *t) {
                OTA::checkOTASync();
                lv_timer_del(t);
            }, 5000, NULL);
            return;
        }
    }
    display_main_menu_buttons();
    screensaver_start_timer();
    // Heartbeat LED - double-pulse on LED 0
    static lv_timer_t *hb_timer = NULL;
    if (hb_timer) lv_timer_del(hb_timer);
    hb_timer = lv_timer_create([](lv_timer_t *t) {
        static uint8_t hb_phase = 0;
        hb_phase = (hb_phase + 1) % 20;
        uint8_t brightness = 0;
        if (hb_phase == 0 || hb_phase == 3) brightness = 30;
        if (hb_phase == 1 || hb_phase == 4) brightness = 15;
        setNeoPixelColor(0, Adafruit_NeoPixel::Color(0, brightness, brightness));
        for (int i = 1; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, 0);
    }, 100, NULL);
}

//----------------------------------------------------
// Create Manual Check-In Window
//----------------------------------------------------
void create_checkin_window() {
    lv_obj_t *checkin_window = create_basic_window();
    load_screen_and_delete_old(checkin_window);

    lv_obj_t *label = lv_label_create(checkin_window);
    lv_label_set_text(label, "Attempting to check in...");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Add spinner (LVGL 8/9: arc or spinner widget)
    lv_obj_t *spinner = lv_spinner_create(checkin_window);
    lv_spinner_set_anim_params(spinner, 1000, 60); // 1s, 60deg arc
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 30);

    // Run check-in and show result
    bool success = false;
    String resultMsg;
    Serial.println("[Menu] Manual check-in triggered.");
    int httpResult = handleBadgeRegistrationWithResult();
    String lastCheckin = getLastCheckinTime();
    // Remove spinner after check-in
    lv_obj_del(spinner);
    if (httpResult == 200 || httpResult == 201) {
        success = true;
        resultMsg = "Check-in successful!\nLast check-in: " + lastCheckin;
    } else if (httpResult > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Check-in failed! HTTP: %d", httpResult);
        resultMsg = String(buf);
    } else {
        resultMsg = "Check-in failed.\nPlease check Wi-Fi and try again.";
    }
    lv_label_set_text(label, resultMsg.c_str());

    create_back_button(checkin_window);
}

// ...existing code...