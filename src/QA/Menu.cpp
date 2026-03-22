// menu.cpp
#include "QA/Menu.h"
#include "QA/ota.hpp"
#include "QA/schedule.h"
#include "QA/Bling.hpp"
#include "includes.h"   // Contains common definitions and includes
#include "Hardware/BadgeRegistration.h"
#include "Hardware/BadgeVersion.h"
#include "Game/LunarState.h"

void create_checkin_window();
void create_credits_window();

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

// Helper to create a modern, white, padded window with row wrap flex
lv_obj_t* create_basic_window() {
    lv_obj_t* win = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(win, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_flex_flow(win, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(win, 10, 0);
    lv_obj_set_style_pad_column(win, 10, 0);
    lv_obj_set_style_pad_all(win, 20, 0);
    return win;
}

lv_obj_t * create_styled_label(lv_obj_t * parent, const char * text, lv_align_t align, int x_offset, int y_offset) {
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
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
void create_back_button(lv_obj_t * parent) {
    lv_obj_t * back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    // When pressed, go back to the main menu
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        if (ota_timer) {
            lv_timer_del(ota_timer);  // Cancel the timer
            ota_timer = nullptr;
        }
        create_main_menu(false);  // Skip the OTA update check
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
}

//----------------------------------------------------
// Create OTA Window
//----------------------------------------------------
void create_ota_window() {
    lv_obj_t *ota_window = create_basic_window();
    lv_scr_load(ota_window);

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
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
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
        lv_obj_set_style_text_color(err_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
        lv_obj_align(err_label, LV_ALIGN_CENTER, 0, 30);
    } else {
        lv_obj_t *up_to_date = lv_label_create(ota_window);
        lv_label_set_text(up_to_date, "You are running the latest version.");
        lv_obj_set_style_text_color(up_to_date, lv_color_hex(0x008000), LV_PART_MAIN);
        lv_obj_align(up_to_date, LV_ALIGN_CENTER, 0, 30);
    }
    create_back_button(ota_window);
}

//----------------------------------------------------
// Create Battery Window
//----------------------------------------------------
void create_battery_window() {
    lv_obj_t *BatteryWindow = create_basic_window();
    lv_scr_load(BatteryWindow);

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

    create_back_button(BatteryWindow);
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
    lv_scr_load(BuzzerWindow);
    
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
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        noTone(BUZZER_PIN);  // Stop any playing tone
        if (ota_timer) {
            lv_timer_del(ota_timer);
            ota_timer = nullptr;
        }
        buzzer_slider_label = nullptr; // Reset label pointer to avoid stale pointer crash
        BuzzerWindow = nullptr;         // Reset window pointer for safety
        create_main_menu(false);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
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
    lv_scr_load(NeoWindow);

    const String buttons[] = {"Red", "Green", "Blue", "Off"};
    for (String name : buttons) {
        lv_obj_t * btn = lv_btn_create(NeoWindow);
        lv_obj_set_size(btn, 120, 50);
        lv_obj_add_event_cb(btn, neopixel_event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, name.c_str());
        lv_obj_center(label);
    }

    create_back_button(NeoWindow);
}

//----------------------------------------------------
//----------------------------------------------------
void create_sd_card_window() {
    SDCardWindow = create_basic_window();
    lv_scr_load(SDCardWindow);

    File root = SD.open("/");
    if (!root) {
        Serial.println("Failed to open directory");
        lv_obj_t * error_label = lv_label_create(SDCardWindow);
        lv_label_set_text(error_label, "Failed to open SD card");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        create_back_button(SDCardWindow);
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        root.close();
        lv_obj_t * error_label = lv_label_create(SDCardWindow);
        lv_label_set_text(error_label, "Root is not a directory");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        create_back_button(SDCardWindow);
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
        lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        file = root.openNextFile();
    }
    
    root.close();
    create_back_button(SDCardWindow);
}

//----------------------------------------------------
// Forward declaration for credits window
void create_credits_window();

//----------------------------------------------------
// Create Credits Window
//----------------------------------------------------
void create_credits_window(){
    CreditsWindow = create_basic_window();
    lv_scr_load(CreditsWindow);


    char buf[300];
    snprintf(buf, sizeof(buf), "Thank you for using the QA Badge!\n\n"
                               "Developed by:\n"
                               "BPLabs\n"
                               "https://bplabs.tech\n\n"
                               "Powered by:\n"
                               "LVGL\n"
                               "https://lvgl.io");

    lv_obj_t *label = lv_label_create(CreditsWindow);
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    create_back_button(CreditsWindow);
}

//----------------------------------------------------
// Create System Info Window
//----------------------------------------------------
void create_system_info_window() {
    SystemWindow = create_basic_window();
    lv_scr_load(SystemWindow);

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
    lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10); // Align to top-left for better readability

    create_back_button(SystemWindow);
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

//----------------------------------------------------
// Display Main Menu Buttons
//----------------------------------------------------
void display_main_menu_buttons() {
    const String buttons[] = {"RGBs", "LEDs", "Buzzer", "SD Card", "Battery", "Wifi", "OTA", "System", "Credits", "Schedule", "Bling", "Check-In", "Lander"};
    int btn_count = sizeof(buttons) / sizeof(buttons[0]);
    int btns_per_row = 3;
    int btn_width = 78;
    int btn_height = 40;
    int h_spacing = 8;
    int v_spacing = 16;
    int y0 = 10;
    int total_width = btns_per_row * btn_width + (btns_per_row - 1) * h_spacing;
    int screen_width = 320; // Adjust if your screen is different
    int x_offset = (screen_width - total_width) / 2;
    for (int i = 0; i < btn_count; ++i) {
        lv_obj_t * btn = create_modern_button(main_menu, buttons[i].c_str(), button_event_handler, i % 10);
        int row = i / btns_per_row;
        int col = i % btns_per_row;
        int x = x_offset + col * (btn_width + h_spacing);
        int y = y0 + row * (btn_height + v_spacing);
        lv_obj_set_size(btn, btn_width, btn_height);
        lv_obj_set_pos(btn, x, y);
    }
}

//----------------------------------------------------
// Create the Main Menu
//----------------------------------------------------
void create_main_menu(bool show_ota_check) {
    init_modern_button_styles(); // Initialize modern button styles for colored buttons
    main_menu = lv_obj_create(NULL);  // Create a new screen
    lv_obj_set_style_bg_color(main_menu, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_scr_load(main_menu);

    lv_obj_set_flex_flow(main_menu, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(main_menu, 10, 0);
    lv_obj_set_style_pad_column(main_menu, 10, 0);
    lv_obj_set_style_pad_all(main_menu, 20, 0);

    if (show_ota_check) {
        if (WiFi.status() == WL_CONNECTED) {
            OTA ota;
            int availableVersion = ota.getAvailableVersion();
            String onlineVersion = availableVersion > 0 ? String(availableVersion / 100) + "." + String((availableVersion / 10) % 10) + "." + String(availableVersion % 10) : "?";
            String localVersion = BADGE_VERSION;
            if (isVersionNewer(onlineVersion, localVersion)) {
                lv_obj_t * countdown_label = lv_label_create(main_menu);
                char buf[100];
                snprintf(buf, sizeof(buf), "Update available! Current: %s, Available: %s. Checking in 5 seconds...", BADGE_VERSION, onlineVersion.c_str());
                lv_label_set_text(countdown_label, buf);
                lv_obj_align(countdown_label, LV_ALIGN_CENTER, 0, 50);

                lv_obj_t * no_btn = lv_btn_create(main_menu);
                lv_obj_set_size(no_btn, 120, 50);
                lv_obj_align(no_btn, LV_ALIGN_CENTER, 0, 100);
                lv_obj_add_event_cb(no_btn, [](lv_event_t * e) {
                    if (ota_timer) {
                        lv_timer_del(ota_timer);  // Cancel the timer
                        ota_timer = nullptr;
                    }
                    lv_obj_clean(main_menu);
                    display_main_menu_buttons();
                }, LV_EVENT_CLICKED, NULL);

                lv_obj_t * no_label = lv_label_create(no_btn);
                lv_label_set_text(no_label, "Skip");

                ota_timer = lv_timer_create([](lv_timer_t * timer) {
                    OTA::checkOTASync();
                    lv_timer_del(timer);  // Delete the timer after it has been triggered
                }, 5000, NULL);
            } else {
                Serial.println("No updates found. Proceeding to menu.");
                display_main_menu_buttons();
            }
        } else {
            Serial.println("No Wi-Fi connection. Skipping OTA update check and proceeding to menu.");
            display_main_menu_buttons();
        }
    } else {
        display_main_menu_buttons();
    }
}

//----------------------------------------------------
// Create Manual Check-In Window
//----------------------------------------------------
void create_checkin_window() {
    lv_obj_t *checkin_window = create_basic_window();
    lv_scr_load(checkin_window);

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