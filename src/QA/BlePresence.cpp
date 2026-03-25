#include "QA/BlePresence.h"
#include "QA/Menu.h"
#include "Hardware/NeoPixelControl.h"
#include "pins.h"

#ifndef NATIVE_TEST

#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <Adafruit_NeoPixel.h>
#include <lvgl.h>

// BSidesKC custom service UUID (16-bit)
#define BSIDES_SERVICE_UUID 0xBD26

static char s_callsign[BLE_CALLSIGN_LEN + 1] = "PILOT";
static uint16_t s_score = 0;
static uint8_t s_status = 0;

static const char* preset_messages[] = {
    "",                          // 0 = none
    "Hello crew!",               // 1
    "Nice badge!",               // 2
    "GG!",                       // 3
    "Need coffee",               // 4
    "CTF team up?",              // 5
    "Good talk!",                // 6
    "After party?",              // 7
    "Ad astra!",                 // 8
    "SOS - need help",           // 9
    "Check the leaderboard",     // 10
    "Lander challenge?",         // 11
    "See you at the bar",        // 12
};

static uint8_t s_msg_id = 0;
static uint32_t s_msg_send_ms = 0;
static uint32_t s_last_send_ms = 0;

static CrewEntry s_crew[BLE_MAX_CREW];
static int s_crew_count = 0;
static int s_nearby = 0;

static BLEScan *pScan = NULL;
static lv_timer_t *scan_timer = NULL;
static bool s_scanning = false;

extern uint32_t badge_boot_ms;

static void start_advertising() {
    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->stop();

    // Manufacturer data: callsign(10) + score(2) + status(1) + msg_id(1)
    uint8_t mfg_data[14];
    memset(mfg_data, 0, sizeof(mfg_data));
    memcpy(mfg_data, s_callsign, strlen(s_callsign));
    mfg_data[10] = s_score & 0xFF;
    mfg_data[11] = (s_score >> 8) & 0xFF;
    mfg_data[12] = s_status;
    mfg_data[13] = s_msg_id;

    BLEAdvertisementData advData;
    advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advData.setCompleteServices(BLEUUID((uint16_t)BSIDES_SERVICE_UUID));
    advData.setManufacturerData(std::string((char *)mfg_data, 14));

    pAdv->setAdvertisementData(advData);
    pAdv->setMinInterval(0x50);
    pAdv->setMaxInterval(0x100);
    pAdv->start();
}

static void process_result(BLEAdvertisedDevice &dev) {
    if (!dev.haveServiceUUID()) return;
    if (!dev.isAdvertisingService(BLEUUID((uint16_t)BSIDES_SERVICE_UUID))) return;

    std::string mfg = dev.getManufacturerData();
    if (mfg.length() < 13) return;

    char callsign[BLE_CALLSIGN_LEN + 1];
    memcpy(callsign, mfg.data(), BLE_CALLSIGN_LEN);
    callsign[BLE_CALLSIGN_LEN] = '\0';
    for (int i = BLE_CALLSIGN_LEN - 1; i >= 0 && callsign[i] == '\0'; i--)
        callsign[i] = '\0';

    if (callsign[0] == '\0') return;
    if (strcmp(callsign, s_callsign) == 0) return;

    uint16_t score = (uint8_t)mfg[10] | ((uint8_t)mfg[11] << 8);
    int8_t rssi = dev.getRSSI();
    uint32_t now = millis();
    uint32_t met = (now - badge_boot_ms) / 1000;

    // Find existing or add new
    int idx = -1;
    for (int i = 0; i < s_crew_count; i++) {
        if (strcmp(s_crew[i].callsign, callsign) == 0) { idx = i; break; }
    }

    if (idx >= 0) {
        s_crew[idx].high_score = score;
        s_crew[idx].rssi = rssi;
        s_crew[idx].last_seen_ms = now;
        s_crew[idx].new_discovery = false;
    } else if (s_crew_count < BLE_MAX_CREW) {
        idx = s_crew_count++;
        strncpy(s_crew[idx].callsign, callsign, BLE_CALLSIGN_LEN);
        s_crew[idx].callsign[BLE_CALLSIGN_LEN] = '\0';
        s_crew[idx].high_score = score;
        s_crew[idx].rssi = rssi;
        s_crew[idx].first_seen_met = met;
        s_crew[idx].last_seen_ms = now;
        s_crew[idx].new_discovery = true;
        s_crew[idx].last_msg_id = 0;
        s_crew[idx].last_msg_ms = 0;

        // Proximity greeting — cyan pulse
        for (int i = 0; i < NUM_NEOPIXELS; i++)
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(0, 60, 80));
    }

    // Parse message_id (byte 14)
    if (idx >= 0) {
        uint8_t msg_id = (mfg.length() >= 14) ? (uint8_t)mfg[13] : 0;
        if (msg_id != 0 && msg_id <= BLE_NUM_MESSAGES) {
            if (s_crew[idx].last_msg_id != msg_id || now - s_crew[idx].last_msg_ms > 10000) {
                s_crew[idx].last_msg_id = msg_id;
                s_crew[idx].last_msg_ms = now;
                // Purple pulse for incoming message
                for (int i = 0; i < NUM_NEOPIXELS; i++)
                    setNeoPixelColor(i, Adafruit_NeoPixel::Color(60, 0, 80));
            }
        }
    }
}

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        process_result(advertisedDevice);
    }
};

static ScanCallbacks scanCb;

// Non-blocking scan complete callback
static void on_scan_complete(BLEScanResults results) {
    s_scanning = false;
}

static void scan_tick(lv_timer_t *t) {
    if (!pScan || s_scanning) return;

    // Clear message after 5 seconds
    if (s_msg_id != 0 && millis() - s_msg_send_ms > 5000) {
        s_msg_id = 0;
        start_advertising();
    }

    // Count nearby (seen in last 30s)
    uint32_t now = millis();
    s_nearby = 0;
    for (int i = 0; i < s_crew_count; i++) {
        if (now - s_crew[i].last_seen_ms < 30000) s_nearby++;
    }

    // Non-blocking scan: pass callback so it doesn't block
    s_scanning = true;
    pScan->start(3, on_scan_complete, false);
}

void ble_presence_init(const char *my_callsign, uint16_t my_score) {
    strncpy(s_callsign, my_callsign, BLE_CALLSIGN_LEN);
    s_callsign[BLE_CALLSIGN_LEN] = '\0';
    s_score = my_score;
    s_crew_count = 0;
    s_nearby = 0;

    BLEDevice::init("BSidesKC");

    pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCb, true);
    pScan->setActiveScan(false);
    pScan->setInterval(100);
    pScan->setWindow(50);

    start_advertising();

    scan_timer = lv_timer_create(scan_tick, 10000, NULL);
}

void ble_presence_update_score(uint16_t score) {
    s_score = score;
    start_advertising();
}

void ble_presence_set_status(uint8_t status) {
    s_status = status;
    start_advertising();
}

int ble_presence_nearby_count() { return s_nearby; }
int ble_presence_total_count() { return s_crew_count; }
const CrewEntry *ble_presence_get_crew() { return s_crew; }
int ble_presence_get_crew_count() { return s_crew_count; }

void ble_presence_send_message(uint8_t msg_id) {
    if (msg_id == 0 || msg_id > BLE_NUM_MESSAGES) return;
    uint32_t now = millis();
    if (now - s_last_send_ms < 30000) return;
    s_msg_id = msg_id;
    s_msg_send_ms = now;
    s_last_send_ms = now;
    start_advertising();
}

const char* ble_presence_get_message_text(uint8_t msg_id) {
    if (msg_id > BLE_NUM_MESSAGES) return "";
    return preset_messages[msg_id];
}

void create_comms_window() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);

    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text(hdr, "COMMS BROADCAST");
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_unscii_8, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 8, 8);

    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 312, 1);
    lv_obj_set_pos(line, 4, 22);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    // Rate limit indicator
    lv_obj_t *rate_label = lv_label_create(scr);
    lv_obj_set_style_text_font(rate_label, &lv_font_unscii_8, 0);
    lv_obj_align(rate_label, LV_ALIGN_TOP_RIGHT, -8, 8);
    uint32_t since = millis() - s_last_send_ms;
    if (since < 30000) {
        lv_label_set_text_fmt(rate_label, "WAIT %ds", (int)((30000 - since) / 1000));
        lv_obj_set_style_text_color(rate_label, lv_color_hex(0x888888), 0);
    } else {
        lv_label_set_text(rate_label, "READY");
        lv_obj_set_style_text_color(rate_label, lv_color_hex(0x00c853), 0);
    }

    // Message buttons: 2 columns x 6 rows
    for (int i = 0; i < BLE_NUM_MESSAGES; i++) {
        int col = i % 2;
        int row = i / 2;
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 152, 26);
        lv_obj_set_pos(btn, 4 + col * 156, 26 + row * 30);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a2e), 0);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            int id = (int)(intptr_t)lv_event_get_user_data(e);
            ble_presence_send_message(id);
            create_comms_window();
        }, LV_EVENT_CLICKED, (void*)(intptr_t)(i + 1));
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, preset_messages[i + 1]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);
        lv_obj_center(lbl);
    }

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

// Crew log screen
void create_crew_log_window() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);

    // Header
    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text_fmt(hdr, "CREW ROSTER  %d/%d", s_nearby, s_crew_count);
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_unscii_8, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 8, 8);

    // Accent line
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 312, 1);
    lv_obj_set_pos(line, 4, 22);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    // Scrollable list
    lv_obj_t *list = lv_obj_create(scr);
    lv_obj_set_size(list, 312, 170);
    lv_obj_set_pos(list, 4, 26);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    if (s_crew_count == 0) {
        lv_obj_t *empty = lv_label_create(list);
        lv_label_set_text(empty, "No crew detected yet.\nWalk around!");
        lv_obj_set_style_text_color(empty, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(empty, &lv_font_unscii_8, 0);
        lv_obj_align(empty, LV_ALIGN_CENTER, 0, 0);
    } else {
        uint32_t now = millis();
        for (int i = 0; i < s_crew_count; i++) {
            lv_obj_t *row = lv_label_create(list);
            bool nearby = (now - s_crew[i].last_seen_ms < 30000);
            uint32_t met = s_crew[i].first_seen_met;
            lv_label_set_text_fmt(row, "%s  %02d:%02d:%02d  %d pts",
                s_crew[i].callsign,
                (int)(met / 3600), (int)((met % 3600) / 60), (int)(met % 60),
                s_crew[i].high_score);
            lv_obj_set_style_text_color(row, nearby ? lv_color_hex(0x00c853) : lv_color_hex(0x666666), 0);
            lv_obj_set_style_text_font(row, &lv_font_unscii_8, 0);
            lv_obj_set_pos(row, 4, i * 16);
        }
    }

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

#else // NATIVE_TEST stubs

void ble_presence_init(const char *, uint16_t) {}
void ble_presence_update_score(uint16_t) {}
void ble_presence_set_status(uint8_t) {}
int ble_presence_nearby_count() { return 0; }
int ble_presence_total_count() { return 0; }
const CrewEntry *ble_presence_get_crew() { return nullptr; }
int ble_presence_get_crew_count() { return 0; }
void create_crew_log_window() {}
void ble_presence_send_message(uint8_t) {}
const char* ble_presence_get_message_text(uint8_t) { return ""; }
void create_comms_window() {}

#endif
