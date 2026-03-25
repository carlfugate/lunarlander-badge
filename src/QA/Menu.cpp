// menu.cpp
#include "QA/Menu.h"
#include "QA/ota.hpp"
#include "QA/schedule.h"
#include "QA/Bling.hpp"
#include "Includes.h"   // Contains common definitions and includes
#include "Hardware/BadgeRegistration.h"
#include "Hardware/BadgeVersion.h"
#include "Game/LunarState.h"
#include "Game/LunarAudio.h"
#include "QA/Screensaver.h"
#include "QA/Callsign.h"
#include "QA/Achievements.h"
#include "QA/Reminder.h"

void create_checkin_window();
void create_credits_window();
void create_system_submenu();

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
lv_obj_t * BuzzerWindow    = nullptr;
lv_obj_t * BatteryWindow   = nullptr;
lv_obj_t * SDCardWindow    = nullptr;
lv_obj_t * SystemWindow    = nullptr;
lv_obj_t * CreditsWindow  = nullptr;
lv_obj_t * buzzer_slider_label = nullptr;
lv_obj_t * ScheduleWindow = nullptr;

bool performOtaUpdateCheck = true;

// File-level timer statics for proper lifecycle management
static lv_timer_t *met_timer = NULL;
static lv_timer_t *hb_timer = NULL;
static lv_timer_t *crawl_timer_global = NULL;

void stop_menu_timers() {
    if (met_timer) { lv_timer_del(met_timer); met_timer = NULL; }
    if (hb_timer) { lv_timer_del(hb_timer); hb_timer = NULL; }
    if (crawl_timer_global) { lv_timer_del(crawl_timer_global); crawl_timer_global = NULL; }
}

static void log_heap(const char *tag) {
    Serial.printf("[HEAP] %s: free=%d, min=%d\n", tag, ESP.getFreeHeap(), ESP.getMinFreeHeap());
}

void load_screen_and_delete_old(lv_obj_t *new_scr) {
    lv_obj_t *old = lv_scr_act();
    lv_scr_load(new_scr);
    if (old && old != new_scr) lv_obj_del(old);
}

void load_screen_and_delete_old_back(lv_obj_t *new_scr) {
    lv_obj_t *old = lv_scr_act();
    lv_scr_load(new_scr);
    if (old && old != new_scr) lv_obj_del(old);
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

// HUD-themed screen: dark bg, cyan title, accent line
static lv_obj_t* create_hud_screen(const char *title) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);
    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text(hdr, title);
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_unscii_8, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 8, 6);
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 312, 1);
    lv_obj_set_pos(line, 4, 22);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    return scr;
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
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -4);
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
    lv_obj_t *ota_window = create_hud_screen("OTA UPDATE");

    // Get current version
    String codeVersion = BADGE_VERSION;

    // Get online version as string
    OTA ota;
    int availableVersion = ota.getAvailableVersion();
    String onlineVersion = availableVersion > 0 ? String(availableVersion / 100) + "." + String((availableVersion / 10) % 10) + "." + String(availableVersion % 10) : "?";
    String localVersion = BADGE_VERSION;

    char buf[128];
    snprintf(buf, sizeof(buf), "Current: %s\nOnline:  %s", localVersion.c_str(), onlineVersion.c_str());
    lv_obj_t *label = lv_label_create(ota_window);
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 28);

    if (isVersionNewer(onlineVersion, localVersion)) {
        lv_obj_t *update_btn = lv_btn_create(ota_window);
        lv_obj_set_size(update_btn, 140, 50);
        lv_obj_align(update_btn, LV_ALIGN_CENTER, 0, 30);
        lv_obj_t *update_label = lv_label_create(update_btn);
        lv_label_set_text(update_label, "Update Now");
        lv_obj_center(update_label);
        lv_obj_add_event_cb(update_btn, [](lv_event_t * e) {
            const lv_obj_t *target = (const lv_obj_t *)lv_event_get_target(e);
            lv_obj_t * parent = lv_obj_get_parent(target);
            lv_obj_t *progress = lv_label_create(parent);
            lv_label_set_text(progress, "Starting OTA update...\nDo not power off.");
            lv_obj_set_style_text_color(progress, lv_color_hex(0x00e5ff), 0);
            lv_obj_align(progress, LV_ALIGN_CENTER, 0, 60);
            lv_refr_now(NULL);
            OTA::checkOTASync();
        }, LV_EVENT_CLICKED, NULL);
    } else if (availableVersion == -1) {
        lv_obj_t *err_label = lv_label_create(ota_window);
        lv_label_set_text(err_label, "Could not check online version.");
        lv_obj_set_style_text_color(err_label, lv_color_hex(0xff4444), 0);
        lv_obj_align(err_label, LV_ALIGN_CENTER, 0, 30);
    } else {
        lv_obj_t *up_to_date = lv_label_create(ota_window);
        lv_label_set_text(up_to_date, "You are running the latest version.");
        lv_obj_set_style_text_color(up_to_date, lv_color_hex(0x00c853), 0);
        lv_obj_align(up_to_date, LV_ALIGN_CENTER, 0, 30);
    }
    create_back_button(ota_window, back_to_system_cb);
}

//----------------------------------------------------
// Create Battery Window
//----------------------------------------------------
void create_battery_window() {
    lv_obj_t *BatteryWindow = create_hud_screen("BATTERY STATUS");

    char buf[100];
    float bat_cent = 0.0;
    float bat_volt = 0.0;
    
    if (max17048_available) {
        bat_cent = max17048.cellPercent();
        bat_volt = max17048.cellVoltage();
        snprintf(buf, sizeof(buf), "Battery: %.2f%%\nVoltage: %.2fV", bat_cent, bat_volt);
    } else {
        snprintf(buf, sizeof(buf), "Battery Monitor\nNot Available\n\nCheck Hardware");
    }

    lv_obj_t *label = lv_label_create(BatteryWindow);
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 28);

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
    BuzzerWindow = create_hud_screen("BUZZER TEST");
    
    // Initialize LEDC for buzzer by briefly setting up a tone channel
    tone(BUZZER_PIN, 1000);
    delay(1);
    noTone(BUZZER_PIN);

    lv_obj_t * slider = lv_slider_create(BuzzerWindow);
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 0, 2000);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    buzzer_slider_label = lv_label_create(BuzzerWindow);
    lv_label_set_text(buzzer_slider_label, "Tone: 0 Hz");
    lv_obj_set_style_text_color(buzzer_slider_label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(buzzer_slider_label, &lv_font_unscii_8, 0);
    lv_obj_align_to(buzzer_slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Custom back button that stops buzzer before returning
    lv_obj_t * back_btn = lv_btn_create(BuzzerWindow);
    lv_obj_set_size(back_btn, 80, 28);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -4);
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
//----------------------------------------------------
void create_sd_card_window() {
    SDCardWindow = create_hud_screen("SD CARD");

    File root = SD.open("/");
    if (!root) {
        Serial.println("Failed to open directory");
        lv_obj_t * error_label = lv_label_create(SDCardWindow);
        lv_label_set_text(error_label, "Failed to open SD card");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xff4444), 0);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        create_back_button(SDCardWindow, back_to_system_cb);
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        root.close();
        lv_obj_t * error_label = lv_label_create(SDCardWindow);
        lv_label_set_text(error_label, "Root is not a directory");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xff4444), 0);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        create_back_button(SDCardWindow, back_to_system_cb);
        return;
    }

    int file_y = 28;
    File file = root.openNextFile();
    while (file) {
        char buf[100];
        if (file.isDirectory()) {
            snprintf(buf, sizeof(buf), "[D] %s", file.name());
        } else {
            snprintf(buf, sizeof(buf), "    %s (%d bytes)", file.name(), file.size());
        }

        lv_obj_t * label = lv_label_create(SDCardWindow);
        lv_label_set_text(label, buf);
        lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
        lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, file_y);
        file_y += 14;

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
    lv_obj_t *scr = create_hud_screen("CREDITS");

    lv_obj_t *t1 = lv_label_create(scr);
    lv_label_set_text(t1, "BSidesKC");
    lv_obj_set_style_text_color(t1, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(t1, LV_ALIGN_TOP_MID, 0, 28);

    // Apollo 11 transcript easter egg: tap title 5 times
    static int credits_tap_count = 0;
    credits_tap_count = 0;
    lv_obj_add_flag(t1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(t1, [](lv_event_t *e) {
        credits_tap_count++;
        if (credits_tap_count >= 5) {
            credits_tap_count = 0;
            lv_obj_t *scr = create_hud_screen("APOLLO 11 TRANSCRIPT");

            lv_obj_t *transcript = lv_label_create(scr);
            lv_label_set_long_mode(transcript, LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_width(transcript, 300);
            lv_label_set_text(transcript,
                "HOUSTON: 60 seconds.    "
                "EAGLE: Got the shadow out there.    "
                "HOUSTON: 30 seconds.    "
                "ALDRIN: Contact light.    "
                "ARMSTRONG: Okay, engine stop.    "
                "ALDRIN: ACA out of detent. Mode control, both auto.    "
                "ALDRIN: Descent engine command override, off. Engine arm, off.    "
                "ARMSTRONG: Houston, Tranquility Base here. The Eagle has landed.    "
                "HOUSTON: Roger, Tranquility, we copy you on the ground. You got a bunch of guys about to turn blue. We're breathing again. Thanks a lot.    "
                "ARMSTRONG: Thank you.    "
                "HOUSTON: You're looking good here.    "
                "ALDRIN: Okay, let's get on with it.    "
                "ARMSTRONG: Okay.    "
            );
            lv_obj_set_style_text_color(transcript, lv_color_hex(0x00c853), 0);
            lv_obj_set_style_anim_duration(transcript, lv_anim_speed(20), 0);
            lv_obj_align(transcript, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t *back = lv_btn_create(scr);
            lv_obj_set_size(back, 80, 28);
            lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -4);
            lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
            lv_obj_add_event_cb(back, [](lv_event_t *e) { create_credits_window(); }, LV_EVENT_CLICKED, NULL);
            lv_obj_t *bl = lv_label_create(back);
            lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
            lv_obj_set_style_text_color(bl, lv_color_hex(0x888888), 0);
            lv_obj_center(bl);
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *t2 = lv_label_create(scr);
    lv_label_set_text(t2, "2 0 2 6");
    lv_obj_set_style_text_color(t2, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(t2, LV_ALIGN_TOP_MID, 0, 46);

    lv_obj_t *t3 = lv_label_create(scr);
    lv_label_set_text(t3, "April 25 - Kansas City");
    lv_obj_set_style_text_color(t3, lv_color_hex(0x888888), 0);
    lv_obj_align(t3, LV_ALIGN_TOP_MID, 0, 64);

    lv_obj_t *body = lv_label_create(scr);
    lv_label_set_text(body, "Badge Hardware\nBadgePirates - bplabs.tech\n\n"
                            "Lunar Lander Game\nAd Astra Protocol\n\n"
                            "Powered by\nLVGL - ESP32-S3");
    lv_obj_set_style_text_color(body, lv_color_hex(0x00c853), 0);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 82);

    lv_obj_t *tag = lv_label_create(scr);
    lv_label_set_text(tag, "#BadgeLife");
    lv_obj_set_style_text_color(tag, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(tag, LV_ALIGN_BOTTOM_MID, 0, -38);

    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x888888), 0);
    lv_obj_center(bl);

    // Credits crawl easter egg - starts after 10s
    if (crawl_timer_global) lv_timer_del(crawl_timer_global);
    crawl_timer_global = lv_timer_create([](lv_timer_t *t) {
        lv_timer_del(t);
        crawl_timer_global = NULL;
        lv_obj_t *scr = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        load_screen_and_delete_old(scr);

        lv_obj_t *crawl = lv_label_create(scr);
        lv_label_set_text(crawl,
            "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
            "A long time ago\n"
            "in a city on the plains...\n\n\n"
            "A band of pirates forged\n"
            "silicon and code into\n"
            "something extraordinary.\n\n\n"
            "The badges were crafted\n"
            "with care, but customs\n"
            "had other plans.\n\n\n"
            "Held at the border,\n"
            "the hardware arrived\n"
            "fashionably late.\n\n\n"
            "But the crew would\n"
            "not be stopped.\n\n\n"
            "They built a game.\n"
            "A lunar lander.\n"
            "For the badge that\n"
            "missed its moment.\n\n\n"
            "Ad astra per aspera.\n"
            "To the stars,\n"
            "through difficulties.\n\n\n\n"
            "BSidesKC 2026\n"
            "Kansas City\n\n"
            "FIN"
        );
        lv_obj_set_style_text_color(crawl, lv_color_hex(0x00e5ff), 0);
        lv_obj_set_style_text_align(crawl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(crawl, 300);
        lv_obj_align(crawl, LV_ALIGN_TOP_MID, 0, 240);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, crawl);
        lv_anim_set_values(&a, 240, -800);
        lv_anim_set_duration(&a, 30000);
        lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
            lv_obj_set_y((lv_obj_t*)obj, v);
        });
        lv_anim_start(&a);

        lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(scr, [](lv_event_t *e) {
            create_credits_window();
        }, LV_EVENT_CLICKED, NULL);
    }, 10000, NULL);
    lv_timer_set_repeat_count(crawl_timer_global, 1);
}

//----------------------------------------------------
// Create System Info Window
//----------------------------------------------------
void create_system_info_window() {
    SystemWindow = create_hud_screen("SYSTEM INFO");

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
        "Version:  %s\nCode:     %s\nMAC:      %s\nReg:      %s\nCheckin:  %s\nWiFi:     %s\n\nHeap Total: %d\nHeap Free:  %d\nHeap Used:  %d (%.1f%%)",
        codeVersion.c_str(), codeName.c_str(), macAddress.c_str(), registrationStatus.c_str(), lastCheckin.c_str(), wifiSSID.c_str(),
        totalHeap, freeHeap, usedHeap, memoryUsagePercent);

    lv_obj_t * label = lv_label_create(SystemWindow);
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 28);

    create_back_button(SystemWindow, back_to_system_cb);
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
// Badge Card Window (#40)
//----------------------------------------------------
void create_badge_card_window() {
    lv_obj_t *scr = create_hud_screen("BADGE CARD");

    // Decorative border
    lv_obj_t *border = lv_obj_create(scr);
    lv_obj_set_size(border, 280, 160);
    lv_obj_align(border, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(border, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(border, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(border, 1, 0);
    lv_obj_set_style_radius(border, 8, 0);
    lv_obj_clear_flag(border, LV_OBJ_FLAG_SCROLLABLE);

    // Callsign in large font
    lv_obj_t *name = lv_label_create(scr);
    lv_label_set_text_fmt(name, "%s", callsign_get());
    lv_obj_set_style_text_font(name, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 40);

    // Badge ID (MAC address)
    lv_obj_t *id = lv_label_create(scr);
    lv_label_set_text_fmt(id, "Badge: %s", WiFi.macAddress().c_str());
    lv_obj_set_style_text_color(id, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(id, &lv_font_unscii_8, 0);
    lv_obj_align(id, LV_ALIGN_TOP_MID, 0, 70);

    // Conference branding
    lv_obj_t *conf = lv_label_create(scr);
    lv_label_set_text(conf, "BSidesKC 2026\nAd Astra");
    lv_obj_set_style_text_color(conf, lv_color_hex(0x00c853), 0);
    lv_obj_set_style_text_align(conf, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(conf, LV_ALIGN_CENTER, 0, 20);

    create_back_button(scr);
}

//----------------------------------------------------
// Display Main Menu Buttons — Concept C: Mission Control HUD
//----------------------------------------------------
void display_main_menu_buttons() {
    stop_menu_timers();

    lv_obj_remove_style_all(main_menu);
    lv_obj_set_style_bg_color(main_menu, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_bg_opa(main_menu, LV_OPA_COVER, 0);
    lv_obj_clear_flag(main_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(main_menu, 0, 0);

    // === Top bar (y=0-22) ===
    lv_obj_t *hdr = lv_obj_create(main_menu);
    lv_obj_set_size(hdr, 320, 22);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = lv_label_create(hdr);
    lv_label_set_text(brand, "BSidesKC '26");
    lv_obj_set_style_text_font(brand, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(brand, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *batt = lv_label_create(hdr);
    if (max17048_available) {
        float pct = max17048.cellPercent();
        if (pct > 100) pct = 100;
        lv_label_set_text_fmt(batt, LV_SYMBOL_WIFI " " LV_SYMBOL_BATTERY_FULL " %d%%", (int)pct);
    } else {
        lv_label_set_text(batt, LV_SYMBOL_WIFI " " LV_SYMBOL_USB);
    }
    lv_obj_set_style_text_color(batt, lv_color_hex(0x555555), 0);
    lv_obj_align(batt, LV_ALIGN_RIGHT_MID, -4, 0);

    static lv_obj_t *met_label = NULL;
    static lv_obj_t *data_label = NULL;
    met_label = lv_label_create(hdr);
    lv_obj_set_style_text_font(met_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(met_label, lv_color_hex(0x00e5ff), 0);
    lv_obj_align(met_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(met_label, "MET 00:00:00");

    // Cyan accent line at y=23
    lv_obj_t *accent = lv_obj_create(main_menu);
    lv_obj_set_size(accent, 320, 1);
    lv_obj_set_pos(accent, 0, 23);
    lv_obj_set_style_bg_color(accent, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 0, 0);
    lv_obj_set_style_pad_all(accent, 0, 0);
    lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE);

    // === MET timer ===
    if (met_timer) lv_timer_del(met_timer);
    met_timer = lv_timer_create([](lv_timer_t *t) {
        if (lv_scr_act() != main_menu) return;
        uint32_t elapsed = (millis() - badge_boot_ms) / 1000;
        uint32_t h = elapsed / 3600;
        uint32_t m = (elapsed % 3600) / 60;
        uint32_t s = elapsed % 60;

        reminder_check(elapsed);

        static bool answer_shown = false;
        if (elapsed == 14520 && !answer_shown) {
            answer_shown = true;
            lv_label_set_text(met_label, "The answer is 42");
            for (int i = 0; i < NUM_NEOPIXELS; i++) setNeoPixelColor(i, 0xFFFFFF);
        } else if (answer_shown && elapsed > 14520 && elapsed <= 14525) {
        } else if (elapsed > 28800) {
            static uint8_t party_hue = 0;
            party_hue += 3;
            lv_obj_set_style_text_color(met_label,
                lv_color_hsv_to_rgb(party_hue, 100, 100), 0);
            lv_label_set_text(met_label, LV_SYMBOL_AUDIO " PARTY");
        } else {
            if (answer_shown && elapsed > 14525) answer_shown = false;
            lv_label_set_text_fmt(met_label, "MET %02lu:%02lu:%02lu %s",
                h, m, s,
                (WiFi.status() == WL_CONNECTED) ? LV_SYMBOL_WIFI : "");
        }

        // Update data panel
        if (data_label) {
            lv_label_set_text_fmt(data_label,
                "CREW: %-8s | GAMES: %d | PATCHES: %d/%d",
                callsign_get(), achievements_games_played(),
                achievements_total(), ACH_COUNT);
        }
    }, 1000, NULL);

    // === Left column: two large stacked buttons (y=26) ===
    lv_obj_t *lander_btn = lv_btn_create(main_menu);
    lv_obj_set_size(lander_btn, 190, 74);
    lv_obj_set_pos(lander_btn, 4, 26);
    lv_obj_set_style_bg_color(lander_btn, lv_color_hex(0x2e7d32), 0);
    lv_obj_set_style_bg_grad_color(lander_btn, lv_color_hex(0x00c853), 0);
    lv_obj_set_style_bg_grad_dir(lander_btn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(lander_btn, 8, 0);
    lv_obj_add_event_cb(lander_btn, [](lv_event_t *e) { lunar_lander_start(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ll = lv_label_create(lander_btn);
    lv_label_set_text(ll, LV_SYMBOL_PLAY " LANDER");
    lv_obj_set_style_text_font(ll, &lv_font_montserrat_20, 0);
    lv_obj_center(ll);

    lv_obj_t *sched_btn = lv_btn_create(main_menu);
    lv_obj_set_size(sched_btn, 190, 74);
    lv_obj_set_pos(sched_btn, 4, 104);
    lv_obj_set_style_bg_color(sched_btn, lv_color_hex(0x0277bd), 0);
    lv_obj_set_style_bg_grad_color(sched_btn, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_bg_grad_dir(sched_btn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(sched_btn, 8, 0);
    lv_obj_add_event_cb(sched_btn, [](lv_event_t *e) {
        loadSchedule();
        displaySchedule();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(sched_btn);
    lv_label_set_text(sl, LV_SYMBOL_LIST " SCHEDULE");
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, 0);
    lv_obj_center(sl);

    // === Right column: 2x3 icon grid ===
    struct { const char *icon; const char *name; lv_event_cb_t cb; } grid[] = {
        {LV_SYMBOL_TINT,     "Bling",    [](lv_event_t *e) { create_bling_window(); }},
        {LV_SYMBOL_WIFI,     "WiFi",     [](lv_event_t *e) { create_wifi_window(); }},
        {LV_SYMBOL_OK,       "Badges",   [](lv_event_t *e) { create_achievements_window(); }},
        {LV_SYMBOL_FILE,     "Credits",  [](lv_event_t *e) { create_credits_window(); }},
        {LV_SYMBOL_SETTINGS, "Settings", [](lv_event_t *e) { create_system_submenu(); }},
        {LV_SYMBOL_EYE_OPEN, "My Card",  [](lv_event_t *e) { create_badge_card_window(); }},
    };
    for (int i = 0; i < 6; i++) {
        int col = i % 2, row = i / 2;
        lv_obj_t *btn = lv_btn_create(main_menu);
        lv_obj_set_size(btn, 54, 46);
        lv_obj_set_pos(btn, 200 + col * 60, 26 + row * 54);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a2e), 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_add_event_cb(btn, grid[i].cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *ic = lv_label_create(btn);
        lv_label_set_text(ic, grid[i].icon);
        lv_obj_set_style_text_color(ic, lv_color_hex(0x00e5ff), 0);
        lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 4);

        lv_obj_t *nm = lv_label_create(btn);
        lv_label_set_text(nm, grid[i].name);
        lv_obj_set_style_text_font(nm, &lv_font_unscii_8, 0);
        lv_obj_set_style_text_color(nm, lv_color_hex(0x888888), 0);
        lv_obj_align(nm, LV_ALIGN_BOTTOM_MID, 0, -2);
    }

    // === Data panel (y=188) ===
    lv_obj_t *data_bg = lv_obj_create(main_menu);
    lv_obj_set_size(data_bg, 312, 20);
    lv_obj_set_pos(data_bg, 4, 188);
    lv_obj_set_style_bg_color(data_bg, lv_color_hex(0x111118), 0);
    lv_obj_set_style_border_width(data_bg, 1, 0);
    lv_obj_set_style_border_color(data_bg, lv_color_hex(0x222233), 0);
    lv_obj_set_style_radius(data_bg, 4, 0);
    lv_obj_set_style_pad_all(data_bg, 0, 0);
    lv_obj_clear_flag(data_bg, LV_OBJ_FLAG_SCROLLABLE);

    data_label = lv_label_create(data_bg);
    lv_obj_set_style_text_font(data_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(data_label, lv_color_hex(0x00e5ff), 0);
    lv_label_set_text_fmt(data_label,
        "CREW: %-8s | GAMES: %d | PATCHES: %d/%d",
        callsign_get(), achievements_games_played(),
        achievements_total(), ACH_COUNT);
    lv_obj_align(data_label, LV_ALIGN_LEFT_MID, 4, 0);

    // === Ticker (y=212) ===
    lv_obj_t *ticker_bg = lv_obj_create(main_menu);
    lv_obj_set_size(ticker_bg, 312, 22);
    lv_obj_set_pos(ticker_bg, 4, 212);
    lv_obj_set_style_bg_color(ticker_bg, lv_color_hex(0x111118), 0);
    lv_obj_set_style_border_width(ticker_bg, 1, 0);
    lv_obj_set_style_border_color(ticker_bg, lv_color_hex(0x222233), 0);
    lv_obj_set_style_radius(ticker_bg, 4, 0);
    lv_obj_set_style_pad_all(ticker_bg, 0, 0);
    lv_obj_clear_flag(ticker_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ticker = lv_label_create(ticker_bg);
    lv_label_set_long_mode(ticker, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(ticker, 298);
    static char ticker_buf[512];
    build_ticker_text(ticker_buf, sizeof(ticker_buf));
    lv_label_set_text(ticker, ticker_buf);
    lv_obj_set_style_text_font(ticker, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(ticker, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_anim_duration(ticker, 30000, 0);
    lv_obj_align(ticker, LV_ALIGN_LEFT_MID, 4, 0);

    // Konami code touch detector
    lv_obj_add_flag(main_menu, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(main_menu, [](lv_event_t *e) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_active(), &p);
        konami_check(touch_to_direction(p.x, p.y));
    }, LV_EVENT_PRESSED, NULL);
}

static void create_screensaver_picker() {
    lv_obj_t *scr = create_hud_screen("SCREENSAVER");

    static const char* names[] = {"Ad Astra", "Matrix", "Terminal", "Lava Lamp"};
    for (int i = 0; i < SS_MODE_COUNT; i++) {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 200, 32);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 28 + i * 38);
        bool active = (screensaver_get_mode() == (ScreensaverMode)i);
        lv_obj_set_style_bg_color(btn, active ? lv_color_hex(0x1a3a1a) : lv_color_hex(0x1a1a2e), 0);
        lv_obj_set_style_border_color(btn, active ? lv_color_hex(0x00c853) : lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_width(btn, active ? 2 : 1, 0);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            int mode = (int)(intptr_t)lv_event_get_user_data(e);
            screensaver_set_mode((ScreensaverMode)mode);
            create_screensaver_picker();
        }, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[i]);
        lv_obj_set_style_text_color(lbl, active ? lv_color_hex(0x00c853) : lv_color_hex(0xcccccc), 0);
        lv_obj_center(lbl);
    }

    create_back_button(scr, [](lv_event_t *e) { create_system_submenu(); });
}

void create_system_submenu() {
    log_heap("enter create_system_submenu");
    lv_obj_t *scr = create_hud_screen("SYSTEM");

    struct SysItem {
        const char *label;
        const char *icon;
        void (*action)();
    };
    static const SysItem items[] = {
        {"Bling",    LV_SYMBOL_TINT,         []() { create_bling_window(); }},
        {"LEDs",     LV_SYMBOL_CHARGE,       []() { ledStatus = !ledStatus; for (int i = 0; i < numLeds; i++) digitalWrite(ledPins[i], ledStatus); }},
        {"Buzzer",   LV_SYMBOL_AUDIO,        []() { create_buzzer_window(); }},
        {"SD Card",  LV_SYMBOL_DRIVE,        []() { create_sd_card_window(); }},
        {"Battery",  LV_SYMBOL_BATTERY_FULL, []() { create_battery_window(); }},
        {"OTA",      LV_SYMBOL_DOWNLOAD,     []() { create_ota_window(); }},
        {"Info",     LV_SYMBOL_LIST,         []() { create_system_info_window(); }},
        {"Callsign", LV_SYMBOL_EDIT,         []() { create_callsign_window(); }},
        {"Screen",   LV_SYMBOL_IMAGE,        []() { create_screensaver_picker(); }},
    };
    int n = sizeof(items) / sizeof(items[0]);

    for (int i = 0; i < n; i++) {
        int col = i % 2;
        int row = i / 2;
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 152, 30);
        lv_obj_set_pos(btn, 4 + col * 156, 26 + row * 34);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a2e), 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            auto fn = (void(*)())lv_event_get_user_data(e);
            fn();
        }, LV_EVENT_CLICKED, (void*)items[i].action);
        lv_obj_t *lbl = lv_label_create(btn);
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %s", items[i].icon, items[i].label);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);
        lv_obj_center(lbl);
    }

    // Sound toggle — special button with mute state
    int snd_row = n / 2;
    int snd_col = n % 2;
    lv_obj_t *snd_btn = lv_btn_create(scr);
    lv_obj_set_size(snd_btn, 152, 30);
    lv_obj_set_pos(snd_btn, 4 + snd_col * 156, 26 + snd_row * 34);
    lv_obj_set_style_bg_color(snd_btn, audio_is_muted() ? lv_color_hex(0x2e1a1a) : lv_color_hex(0x1a2e1a), 0);
    lv_obj_set_style_radius(snd_btn, 6, 0);
    lv_obj_add_event_cb(snd_btn, [](lv_event_t *e) {
        audio_set_mute(!audio_is_muted());
        audio_click();
        create_system_submenu();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *snd_lbl = lv_label_create(snd_btn);
    lv_label_set_text(snd_lbl, audio_is_muted() ? LV_SYMBOL_MUTE " Muted" : LV_SYMBOL_AUDIO " Sound");
    lv_obj_set_style_text_color(snd_lbl, audio_is_muted() ? lv_color_hex(0xff6666) : lv_color_hex(0x66ff66), 0);
    lv_obj_center(snd_lbl);

    // Back button
    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -4);
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
                if (hb_timer) lv_timer_del(hb_timer);
                hb_timer = lv_timer_create([](lv_timer_t *t) {
        if (lv_scr_act() != main_menu) return;
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
    if (hb_timer) lv_timer_del(hb_timer);
    hb_timer = lv_timer_create([](lv_timer_t *t) {
        if (lv_scr_act() != main_menu) return;
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
    lv_obj_t *checkin_window = create_hud_screen("CHECK-IN");

    lv_obj_t *label = lv_label_create(checkin_window);
    lv_label_set_text(label, "Attempting to check in...");
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 28);

    lv_obj_t *spinner = lv_spinner_create(checkin_window);
    lv_spinner_set_anim_params(spinner, 1000, 60);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 30);

    bool success = false;
    String resultMsg;
    Serial.println("[Menu] Manual check-in triggered.");
    int httpResult = handleBadgeRegistrationWithResult();
    String lastCheckin = getLastCheckinTime();
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
    lv_obj_set_style_text_color(label, success ? lv_color_hex(0x00c853) : lv_color_hex(0xff4444), 0);

    create_back_button(checkin_window);
}

// ...existing code...