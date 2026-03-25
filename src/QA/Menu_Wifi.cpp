#include "QA/Menu.h"
#include "Includes.h"
#include <set>

// Define WiFi-related global variables
lv_obj_t * WifiWindow;
lv_obj_t * WifiScanWindow;
lv_obj_t * keyboard;
lv_obj_t * password_textarea;
String selected_ssid;

// Function declarations
void create_wifi_window();
void create_wifi_scan_window();
void ssid_selected_event_handler(lv_event_t * e);
void connect_to_wifi(const String& ssid, const String& password);

// HUD-themed screen: dark bg, cyan title, accent line
static lv_obj_t* create_wifi_screen(const char *title) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    // Title
    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text(hdr, title);
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
    return scr;
}

//----------------------------------------------------
// Create WiFi Window — COMMS STATUS
//----------------------------------------------------
void create_wifi_window() {
    WifiWindow = create_wifi_screen("COMMS STATUS");
    load_screen_and_delete_old(WifiWindow);

    String ssid   = getConnectedWiFiSSID();
    String status = getWiFiConnectionStatus();
    String ip     = getWiFiIPAddress();

    // Status info
    lv_obj_t * label = lv_label_create(WifiWindow);
    char buf[150];
    snprintf(buf, sizeof(buf), "SSID: %s\nStatus: %s\nIP: %s", ssid.c_str(), status.c_str(), ip.c_str());
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 8, 28);

    // Scan button
    lv_obj_t * scan_btn = lv_btn_create(WifiWindow);
    lv_obj_set_size(scan_btn, 120, 36);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(scan_btn, [](lv_event_t * e) {
        create_wifi_scan_window();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "Scan WiFi");
    lv_obj_set_style_text_color(scan_label, lv_color_hex(0x00e5ff), 0);
    lv_obj_center(scan_label);

    // Back → main menu
    create_back_button(WifiWindow);
}

//----------------------------------------------------
// Create WiFi Scan Window — NETWORK SCAN
//----------------------------------------------------
void create_wifi_scan_window() {
    WifiScanWindow = create_wifi_screen("NETWORK SCAN");
    load_screen_and_delete_old(WifiScanWindow);

    // Scrollable list container below accent line
    lv_obj_t * list = lv_obj_create(WifiScanWindow);
    lv_obj_set_size(list, 304, 170);
    lv_obj_set_pos(list, 8, 26);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 4, 0);
    lv_obj_set_style_pad_all(list, 4, 0);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x0a0a0f), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    // Scanning status
    lv_obj_t * status_label = lv_label_create(list);
    lv_label_set_text(status_label, "Scanning for networks...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_unscii_8, 0);

    lv_refr_now(NULL);

    // WiFi scan — delay() is OK here as a one-time blocking operation
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    char scan_text[50];
    for(int i = 0; i < 3; i++) {
        sprintf(scan_text, "Scanning for networks%.*s", i+1, "...");
        lv_label_set_text(status_label, scan_text);
        lv_refr_now(NULL);
        delay(500); // blocking scan animation — acceptable outside LVGL event loop
    }

    int n = WiFi.scanNetworks();

    // Clear list container for results
    lv_obj_clean(list);

    if (n == 0) {
        lv_obj_t * lbl = lv_label_create(list);
        lv_label_set_text(lbl, "No networks found");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xff4444), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_unscii_8, 0);
    } else {
        lv_obj_t * count_lbl = lv_label_create(list);
        char buf[32];
        snprintf(buf, sizeof(buf), "Found %d networks:", n);
        lv_label_set_text(count_lbl, buf);
        lv_obj_set_style_text_color(count_lbl, lv_color_hex(0x00e5ff), 0);
        lv_obj_set_style_text_font(count_lbl, &lv_font_unscii_8, 0);

        std::set<String> unique_ssids;
        int count = 0;
        for (int i = 0; i < n && count < 5; ++i) {
            String ssid = WiFi.SSID(i);
            if (unique_ssids.find(ssid) == unique_ssids.end()) {
                unique_ssids.insert(ssid);

                lv_obj_t * btn = lv_btn_create(list);
                lv_obj_set_size(btn, 290, 28);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), 0);
                lv_obj_add_event_cb(btn, ssid_selected_event_handler, LV_EVENT_CLICKED, NULL);

                char buf[100];
                snprintf(buf, sizeof(buf), "%s (%ddBm)", ssid.c_str(), WiFi.RSSI(i));
                lv_obj_t * label = lv_label_create(btn);
                lv_label_set_text(label, buf);
                lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
                lv_obj_center(label);

                count++;
            }
        }
    }

    // Back → WiFi main window
    create_back_button(WifiScanWindow, [](lv_event_t * e) { create_wifi_window(); });
}

//----------------------------------------------------
// SSID selected — AUTHENTICATE screen
//----------------------------------------------------
void ssid_selected_event_handler(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    String fullText = String(lv_label_get_text(lv_obj_get_child(btn, 0)));
    int parenIndex = fullText.indexOf(" (");
    selected_ssid = (parenIndex >= 0) ? fullText.substring(0, parenIndex) : fullText;

    lv_obj_t * pw_scr = create_wifi_screen("AUTHENTICATE");
    load_screen_and_delete_old(pw_scr);

    // SSID label
    lv_obj_t * ssid_label = lv_label_create(pw_scr);
    lv_label_set_text_fmt(ssid_label, "Network: %s", selected_ssid.c_str());
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(ssid_label, &lv_font_unscii_8, 0);
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 8, 28);

    // Password text area
    password_textarea = lv_textarea_create(pw_scr);
    lv_textarea_set_password_mode(password_textarea, true);
    lv_obj_set_size(password_textarea, 200, 36);
    lv_obj_align(password_textarea, LV_ALIGN_TOP_MID, 0, 44);
    lv_obj_set_style_bg_color(password_textarea, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_text_color(password_textarea, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_border_color(password_textarea, lv_color_hex(0x00e5ff), 0);

    // Keyboard
    keyboard = lv_keyboard_create(pw_scr);
    lv_keyboard_set_textarea(keyboard, password_textarea);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -36);

    // Connect button
    lv_obj_t * connect_btn = lv_btn_create(pw_scr);
    lv_obj_set_size(connect_btn, 100, 28);
    lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(connect_btn, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(connect_btn, [](lv_event_t * e) {
        String password = lv_textarea_get_text(password_textarea);
        connect_to_wifi(selected_ssid, password);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, "CONNECT");
    lv_obj_set_style_text_color(connect_label, lv_color_hex(0x00e5ff), 0);
    lv_obj_center(connect_label);
}

//----------------------------------------------------
// Connect to WiFi — ESTABLISHING LINK
//----------------------------------------------------
void connect_to_wifi(const String& ssid, const String& password) {
    lv_obj_t * conn_scr = create_wifi_screen("ESTABLISHING LINK");
    load_screen_and_delete_old(conn_scr);

    // Status label
    lv_obj_t * status_label = lv_label_create(conn_scr);
    lv_label_set_text_fmt(status_label, "Connecting to %s...", ssid.c_str());
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_unscii_8, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 8, 28);

    lv_timer_handler();

    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100); // one-time blocking — required for WiFi state reset

    Serial.printf("Attempting to connect to SSID: '%s'\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    const int maxAttempts = 20;
    bool connected = false;
    String error_msg = "";

    while (attempts < maxAttempts) {
        wl_status_t status = WiFi.status();
        delay(500); // blocking WiFi poll — acceptable during connection sequence
        status = WiFi.status();

        switch (status) {
            case WL_CONNECTED:
                connected = true;
                goto connection_complete;
            case WL_NO_SSID_AVAIL:
                delay(1000);
                if (WiFi.status() == WL_CONNECTED) { connected = true; goto connection_complete; }
                error_msg = "Network not found";
                goto connection_complete;
            case WL_CONNECT_FAILED:
                delay(1000);
                if (WiFi.status() == WL_CONNECTED) { connected = true; goto connection_complete; }
                error_msg = "Connection failed - check password";
                goto connection_complete;
            case WL_DISCONNECTED:
            case WL_IDLE_STATUS:
                break;
            default:
                delay(1000);
                if (WiFi.status() == WL_CONNECTED) { connected = true; goto connection_complete; }
                error_msg = "Unknown error: " + String(status);
                goto connection_complete;
        }

        delay(1000); // blocking poll interval — acceptable during connection sequence
        Serial.print(".");
        attempts++;

        lv_label_set_text_fmt(status_label, "Connecting to %s...\nAttempt %d/%d",
            ssid.c_str(), attempts, maxAttempts);
        lv_timer_handler();
    }

    error_msg = "Connection timed out";

connection_complete:
    if (connected && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
        Serial.print("\nConnected to ");
        Serial.println(ssid);
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        saveWiFiCredentialsToSD(ssid.c_str(), password.c_str());

        lv_label_set_text_fmt(status_label, "LINK ESTABLISHED\n\nSSID: %s\nIP: %s",
            ssid.c_str(), WiFi.localIP().toString().c_str());
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00c853), 0);
    } else {
        Serial.print("\nFailed to connect: ");
        Serial.println(error_msg);
        setSolidRed();

        lv_label_set_text_fmt(status_label, "LINK FAILED\n\nSSID: %s\n%s",
            ssid.c_str(), error_msg.c_str());
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xff4444), 0);
    }

    // Back → WiFi main window
    create_back_button(conn_scr, [](lv_event_t * e) { create_wifi_window(); });
}
