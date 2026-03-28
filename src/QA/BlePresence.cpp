#include "QA/BlePresence.h"
#include "FeatureFlags.h"
#include "QA/Menu.h"
#include "Hardware/NeoPixelControl.h"
#include "pins.h"

#ifndef NATIVE_TEST

#include <NimBLEDevice.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <lvgl.h>

// BSidesKC custom service UUID (16-bit)
#define BSIDES_SERVICE_UUID 0xBD26

// Conference-wide secret — baked into firmware, not public
#define BLE_AUTH_KEY 0xB51D  // "BSID" — change before conference

static char s_callsign[BLE_CALLSIGN_LEN + 1] = "PILOT";
static uint16_t s_score = 0;
static uint8_t s_status = 0;

static uint16_t compute_tag(const char *callsign, uint16_t score) {
    uint16_t hash = BLE_AUTH_KEY;
    for (int i = 0; i < BLE_CALLSIGN_LEN && callsign[i]; i++) {
        hash ^= ((uint16_t)callsign[i] << (i % 8));
        hash = (hash << 3) | (hash >> 13); // rotate
    }
    hash ^= score;
    hash ^= (hash >> 8);
    return hash;
}

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

static char s_last_notification[64] = "";
static bool s_has_notification = false;

static uint8_t s_msg_id = 0;
static uint32_t s_msg_send_ms = 0;
static uint32_t s_last_send_ms = 0;

#define MSG_HISTORY_SIZE 20
static struct { char sender[BLE_CALLSIGN_LEN+1]; uint8_t msg_id; uint32_t met; } s_msg_history[MSG_HISTORY_SIZE];
static int s_msg_history_count = 0;
static int s_msg_history_head = 0;

static CrewEntry s_crew[BLE_MAX_CREW];
static int s_crew_count = 0;
static int s_nearby = 0;

static void crew_save() {
    Preferences prefs;
    prefs.begin("crew", false);
    prefs.putUChar("count", s_crew_count);
    prefs.putBytes("data", s_crew, sizeof(CrewEntry) * s_crew_count);
    prefs.end();
}

static void crew_load() {
    Preferences prefs;
    prefs.begin("crew", true);
    s_crew_count = prefs.getUChar("count", 0);
    if (s_crew_count > BLE_MAX_CREW) s_crew_count = 0;
    if (s_crew_count > 0) {
        prefs.getBytes("data", s_crew, sizeof(CrewEntry) * s_crew_count);
    }
    prefs.end();
}

static BLEScan *pScan = NULL;
static lv_timer_t *scan_timer = NULL;
static bool s_scanning = false;

extern uint32_t badge_boot_ms;

static volatile bool s_new_discovery_pulse = false;
static volatile bool s_msg_received_pulse = false;

static void start_advertising() {
    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->stop();

    // Manufacturer data: company_id(2) + callsign(10) + score(2) + status(1) + msg_id(1) + tag(2)
    uint8_t mfg_data[18];
    mfg_data[0] = 0xFF;  // company ID low byte (0xFFFF = BLE SIG "testing")
    mfg_data[1] = 0xFF;  // company ID high byte
    memset(mfg_data + 2, 0, 16);
    memcpy(mfg_data + 2, s_callsign, strlen(s_callsign));
    mfg_data[12] = s_score & 0xFF;
    mfg_data[13] = (s_score >> 8) & 0xFF;
    mfg_data[14] = s_status;
    mfg_data[15] = s_msg_id;
    uint16_t tag = compute_tag(s_callsign, s_score);
    mfg_data[16] = tag & 0xFF;
    mfg_data[17] = (tag >> 8) & 0xFF;

    BLEAdvertisementData advData;
    advData.setFlags(0x06);  // General Discoverable + BR/EDR Not Supported
    advData.setCompleteServices(BLEUUID((uint16_t)BSIDES_SERVICE_UUID));
    advData.setManufacturerData(std::string((char *)mfg_data, 18));

    pAdv->setAdvertisementData(advData);
    pAdv->setMinInterval(0x50);
    pAdv->setMaxInterval(0x100);
    pAdv->start();
}

static void process_result(BLEAdvertisedDevice *dev) {
    if (!dev->haveServiceUUID()) return;
    if (!dev->isAdvertisingService(BLEUUID((uint16_t)BSIDES_SERVICE_UUID))) return;

    std::string mfg = dev->getManufacturerData();
    // NimBLE getManufacturerData() includes 2-byte company ID prefix; skip it
    if (mfg.length() < 2) return;
    const char *data = mfg.data() + 2;
    int data_len = mfg.length() - 2;

#ifdef FF_BLE_NO_AUTH
    if (data_len < 13) return; // minimum: callsign(10) + score(2) + status(1)
#else
    if (data_len < 16) return; // reject short payloads (no auth tag)
#endif

    char callsign[BLE_CALLSIGN_LEN + 1];
    memcpy(callsign, data, BLE_CALLSIGN_LEN);
    callsign[BLE_CALLSIGN_LEN] = '\0';
    for (int i = BLE_CALLSIGN_LEN - 1; i >= 0 && callsign[i] == '\0'; i--)
        callsign[i] = '\0';

    // Validate callsign: printable ASCII only, non-empty
    if (callsign[0] == '\0') return;
    for (int i = 0; callsign[i]; i++) {
        if (callsign[i] < 0x20 || callsign[i] > 0x7E) return;
    }

    if (strcmp(callsign, s_callsign) == 0) return;

    uint16_t score = (uint8_t)data[10] | ((uint8_t)data[11] << 8);

    // Validate score range
    if (score > 10000) return;

    // Verify authentication tag
#ifndef FF_BLE_NO_AUTH
    uint16_t received_tag = (uint8_t)data[14] | ((uint8_t)data[15] << 8);
    uint16_t expected_tag = compute_tag(callsign, score);
    if (received_tag != expected_tag) return;
#endif

    int8_t rssi = dev->getRSSI();
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
        // Flood protection: max 3 new discoveries per 5 seconds
        static uint32_t discovery_times[3] = {0};
        static int discovery_idx = 0;
        if (now - discovery_times[discovery_idx] < 5000) return;
        discovery_times[discovery_idx] = now;
        discovery_idx = (discovery_idx + 1) % 3;

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

        // Defer LED pulse to main task via scan_tick
        s_new_discovery_pulse = true;

        crew_save();
    }

    // Parse message_id (byte 14) with validation
    if (idx >= 0) {
        uint8_t msg_id = (uint8_t)data[13];
        if (msg_id > BLE_NUM_MESSAGES) msg_id = 0; // clamp invalid
        if (msg_id != 0 && msg_id <= BLE_NUM_MESSAGES) {
            if (s_crew[idx].last_msg_id != msg_id || now - s_crew[idx].last_msg_ms > 10000) {
                s_crew[idx].last_msg_id = msg_id;
                s_crew[idx].last_msg_ms = now;
                snprintf(s_last_notification, sizeof(s_last_notification),
                    "%s: %s", callsign, preset_messages[msg_id]);
                s_has_notification = true;
                // Record in message history
                strncpy(s_msg_history[s_msg_history_head].sender, callsign, BLE_CALLSIGN_LEN);
                s_msg_history[s_msg_history_head].sender[BLE_CALLSIGN_LEN] = '\0';
                s_msg_history[s_msg_history_head].msg_id = msg_id;
                s_msg_history[s_msg_history_head].met = met;
                s_msg_history_head = (s_msg_history_head + 1) % MSG_HISTORY_SIZE;
                if (s_msg_history_count < MSG_HISTORY_SIZE) s_msg_history_count++;
                // Defer LED pulse to main task via scan_tick
                s_msg_received_pulse = true;
            }
        }
    }
}

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice* advertisedDevice) override {
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

    // Handle deferred LED pulses from BLE callback (main task safe)
    if (s_new_discovery_pulse) {
        s_new_discovery_pulse = false;
        for (int i = 0; i < NUM_NEOPIXELS; i++)
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(0, 60, 80));
    }
    if (s_msg_received_pulse) {
        s_msg_received_pulse = false;
        for (int i = 0; i < NUM_NEOPIXELS; i++)
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(60, 0, 80));
    }

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
    s_nearby = 0;

    crew_load();

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

void ble_presence_update_callsign(const char *name) {
    strncpy(s_callsign, name, BLE_CALLSIGN_LEN);
    s_callsign[BLE_CALLSIGN_LEN] = '\0';
    start_advertising();
}

int ble_presence_msg_history_count() { return s_msg_history_count; }

const char* ble_presence_msg_history_sender(int idx) {
    if (idx < 0 || idx >= s_msg_history_count) return "";
    int pos = (s_msg_history_head - 1 - idx + MSG_HISTORY_SIZE) % MSG_HISTORY_SIZE;
    return s_msg_history[pos].sender;
}

uint8_t ble_presence_msg_history_msg_id(int idx) {
    if (idx < 0 || idx >= s_msg_history_count) return 0;
    int pos = (s_msg_history_head - 1 - idx + MSG_HISTORY_SIZE) % MSG_HISTORY_SIZE;
    return s_msg_history[pos].msg_id;
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

void ble_presence_stop() {
    if (scan_timer) { lv_timer_del(scan_timer); scan_timer = NULL; }
    BLEDevice::deinit(true);
    pScan = NULL;
    s_scanning = false;
}

void ble_presence_restart() {
    if (pScan) return;  // already running
    BLEDevice::init("BSidesKC");
    pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCb, true);
    pScan->setActiveScan(false);
    pScan->setInterval(100);
    pScan->setWindow(50);
    start_advertising();
    scan_timer = lv_timer_create(scan_tick, 10000, NULL);
}

bool ble_presence_has_notification() { return s_has_notification; }
const char* ble_presence_get_notification() { return s_last_notification; }
void ble_presence_clear_notification() { s_has_notification = false; }

int8_t ble_presence_get_rssi(const char *callsign) {
    for (int i = 0; i < s_crew_count; i++) {
        if (strcmp(s_crew[i].callsign, callsign) == 0) {
            if (millis() - s_crew[i].last_seen_ms < 30000) return s_crew[i].rssi;
            return 0;
        }
    }
    return 0;
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

    // Message history
    int hist_count = ble_presence_msg_history_count();
    if (hist_count > 10) hist_count = 10;
    if (hist_count > 0) {
        lv_obj_t *hist_hdr = lv_label_create(scr);
        lv_label_set_text(hist_hdr, "RECENT");
        lv_obj_set_style_text_color(hist_hdr, lv_color_hex(0x00e5ff), 0);
        lv_obj_set_style_text_font(hist_hdr, &lv_font_unscii_8, 0);
        lv_obj_set_pos(hist_hdr, 8, 210);

        lv_obj_t *hist_list = lv_obj_create(scr);
        lv_obj_set_size(hist_list, 312, 70);
        lv_obj_set_pos(hist_list, 4, 222);
        lv_obj_set_style_bg_color(hist_list, lv_color_hex(0x0a0a0f), 0);
        lv_obj_set_style_border_width(hist_list, 0, 0);
        lv_obj_set_style_pad_all(hist_list, 0, 0);
        lv_obj_set_scroll_dir(hist_list, LV_DIR_VER);

        for (int i = 0; i < hist_count; i++) {
            lv_obj_t *row = lv_label_create(hist_list);
            uint8_t mid = ble_presence_msg_history_msg_id(i);
            lv_label_set_text_fmt(row, "%s: %s",
                ble_presence_msg_history_sender(i),
                (mid <= BLE_NUM_MESSAGES) ? preset_messages[mid] : "");
            lv_obj_set_style_text_color(row, lv_color_hex(0x888888), 0);
            lv_obj_set_style_text_font(row, &lv_font_unscii_8, 0);
            lv_obj_set_pos(row, 4, i * 14);
        }
    }

    // Back button (comms)
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

// Find friend screen
static char s_find_target[BLE_CALLSIGN_LEN + 1] = "";
static lv_obj_t *s_find_bar = NULL;
static lv_obj_t *s_find_rssi_label = NULL;
static lv_obj_t *s_find_hint_label = NULL;
static lv_timer_t *s_find_timer = NULL;

static void find_timer_cb(lv_timer_t *t) {
    if (lv_scr_act() != lv_obj_get_parent(s_find_bar)) {
        lv_timer_del(t); s_find_timer = NULL; return;
    }
    int8_t rssi = ble_presence_get_rssi(s_find_target);
    if (rssi == 0) {
        lv_bar_set_value(s_find_bar, -100, LV_ANIM_ON);
        lv_label_set_text(s_find_rssi_label, "Not nearby");
        lv_obj_set_style_text_color(s_find_rssi_label, lv_color_hex(0x888888), 0);
        lv_label_set_text(s_find_hint_label, "");
    } else {
        lv_bar_set_value(s_find_bar, rssi, LV_ANIM_ON);
        lv_label_set_text_fmt(s_find_rssi_label, "%d dBm", rssi);
        if (rssi > -40) {
            lv_obj_set_style_text_color(s_find_rssi_label, lv_color_hex(0x00c853), 0);
            lv_label_set_text(s_find_hint_label, "Very close!");
        } else if (rssi > -60) {
            lv_obj_set_style_text_color(s_find_rssi_label, lv_color_hex(0x00e5ff), 0);
            lv_label_set_text(s_find_hint_label, "Nearby");
        } else if (rssi > -80) {
            lv_obj_set_style_text_color(s_find_rssi_label, lv_color_hex(0xffab00), 0);
            lv_label_set_text(s_find_hint_label, "In range");
        } else {
            lv_obj_set_style_text_color(s_find_rssi_label, lv_color_hex(0xff4444), 0);
            lv_label_set_text(s_find_hint_label, "Far away");
        }
    }
}

void create_find_friend_window(const char *callsign) {
    strncpy(s_find_target, callsign, BLE_CALLSIGN_LEN);
    s_find_target[BLE_CALLSIGN_LEN] = '\0';

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);

    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text_fmt(hdr, "TRACKING: %s", s_find_target);
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

    s_find_bar = lv_bar_create(scr);
    lv_obj_set_size(s_find_bar, 280, 30);
    lv_obj_align(s_find_bar, LV_ALIGN_CENTER, 0, -20);
    lv_bar_set_range(s_find_bar, -100, -20);
    lv_obj_set_style_bg_color(s_find_bar, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_find_bar, lv_color_hex(0x00e5ff), LV_PART_INDICATOR);

    s_find_rssi_label = lv_label_create(scr);
    lv_obj_set_style_text_font(s_find_rssi_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_find_rssi_label, lv_color_hex(0xcccccc), 0);
    lv_obj_align(s_find_rssi_label, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text(s_find_rssi_label, "Searching...");

    s_find_hint_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_find_hint_label, lv_color_hex(0x888888), 0);
    lv_obj_align(s_find_hint_label, LV_ALIGN_CENTER, 0, 60);
    lv_label_set_text(s_find_hint_label, "");

    if (s_find_timer) lv_timer_del(s_find_timer);
    s_find_timer = lv_timer_create(find_timer_cb, 2000, NULL);

    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { create_crew_log_window(); }, LV_EVENT_CLICKED, NULL);
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
            lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(row, [](lv_event_t *e) {
                const char *name = (const char *)lv_event_get_user_data(e);
                create_find_friend_window(name);
            }, LV_EVENT_CLICKED, (void*)s_crew[i].callsign);
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
int8_t ble_presence_get_rssi(const char *) { return 0; }
void create_find_friend_window(const char *) {}
void ble_presence_send_message(uint8_t) {}
const char* ble_presence_get_message_text(uint8_t) { return ""; }
void create_comms_window() {}
void ble_presence_stop() {}
void ble_presence_restart() {}

void ble_presence_update_callsign(const char*) {}
int ble_presence_msg_history_count() { return 0; }
const char* ble_presence_msg_history_sender(int) { return ""; }
uint8_t ble_presence_msg_history_msg_id(int) { return 0; }

bool ble_presence_has_notification() { return false; }
const char* ble_presence_get_notification() { return ""; }
void ble_presence_clear_notification() {}

#endif

