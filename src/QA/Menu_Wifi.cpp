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

// Helper: create a dark-themed screen
static lv_obj_t* create_dark_screen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    return scr;
}

//----------------------------------------------------
// Create WiFi Window
//----------------------------------------------------
void create_wifi_window() {
    WifiWindow = create_dark_screen();
    load_screen_and_delete_old(WifiWindow);

    String ssid   = getConnectedWiFiSSID();
    String status = getWiFiConnectionStatus();
    String ip     = getWiFiIPAddress();

    // Create container for content
    lv_obj_t * cont = lv_obj_create(WifiWindow);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x0a0a0f), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, 0);

    // Status label
    lv_obj_t * label = lv_label_create(cont);
    char buf[150];
    snprintf(buf, sizeof(buf), "SSID: %s\nStatus: %s\nIP: %s", ssid.c_str(), status.c_str(), ip.c_str());
    lv_label_set_text(label, buf);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), LV_PART_MAIN);

    // Scan button
    lv_obj_t * scan_btn = lv_btn_create(cont);
    lv_obj_set_size(scan_btn, 120, 36);
    lv_obj_add_event_cb(scan_btn, [](lv_event_t * e) {
        create_wifi_scan_window();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "Scan WiFi");
    lv_obj_center(scan_label);

    // Back → main menu
    create_back_button(WifiWindow);
}

//----------------------------------------------------
// Create WiFi Scan Window
//----------------------------------------------------
void create_wifi_scan_window() {
    WifiScanWindow = create_dark_screen();
    load_screen_and_delete_old(WifiScanWindow);

    // Create a container with flex layout
    lv_obj_t * flex_col = lv_obj_create(WifiScanWindow);
    lv_obj_set_size(flex_col, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(flex_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(flex_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(flex_col, lv_color_hex(0x0a0a0f), LV_PART_MAIN);
    lv_obj_set_style_border_width(flex_col, 0, 0);
    lv_obj_set_style_pad_row(flex_col, 4, 0);
    lv_obj_set_scroll_dir(flex_col, LV_DIR_VER);

    // Create scanning status label
    lv_obj_t * status_label = lv_label_create(flex_col);
    lv_label_set_text(status_label, "Scanning for networks...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xcccccc), LV_PART_MAIN);

    // Force screen update
    lv_refr_now(NULL);
    
    // Initialize WiFi for scanning
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Show scanning progress with dots
    char scan_text[50];
    for(int i = 0; i < 3; i++) {
        sprintf(scan_text, "Scanning for networks%.*s", i+1, "...");
        lv_label_set_text(status_label, scan_text);
        lv_refr_now(NULL);
        delay(500);
    }

    // Perform the scan
    int n = WiFi.scanNetworks();
    
    // Clear flex container
    lv_obj_clean(flex_col);

    if (n == 0) {
        lv_obj_t * no_networks_label = lv_label_create(flex_col);
        lv_label_set_text(no_networks_label, "No networks found");
        lv_obj_set_style_text_color(no_networks_label, lv_color_hex(0xcccccc), LV_PART_MAIN);
    } else {
        // Show network count
        lv_obj_t * networks_found_label = lv_label_create(flex_col);
        char buf[32];
        snprintf(buf, sizeof(buf), "Found %d networks:", n);
        lv_label_set_text(networks_found_label, buf);
        lv_obj_set_style_text_color(networks_found_label, lv_color_hex(0x00e5ff), LV_PART_MAIN);
        
        // Show available networks
        std::set<String> unique_ssids;
        int count = 0;
        for (int i = 0; i < n && count < 5; ++i) {
            String ssid = WiFi.SSID(i);
            if (unique_ssids.find(ssid) == unique_ssids.end()) {
                unique_ssids.insert(ssid);
                
                lv_obj_t * btn = lv_btn_create(flex_col);
                lv_obj_set_size(btn, 200, 32);
                lv_obj_add_event_cb(btn, ssid_selected_event_handler, LV_EVENT_CLICKED, NULL);
                
                char buf[100];
                snprintf(buf, sizeof(buf), "%s (%ddBm)", ssid.c_str(), WiFi.RSSI(i));
                lv_obj_t * label = lv_label_create(btn);
                lv_label_set_text(label, buf);
                lv_obj_center(label);

                count++;
            }
        }
    }
    // Back → WiFi main window
    create_back_button(WifiScanWindow, [](lv_event_t * e) { create_wifi_window(); });
}

//----------------------------------------------------
// Event handler for selecting a WiFi network
//----------------------------------------------------
void ssid_selected_event_handler(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    String fullText = String(lv_label_get_text(lv_obj_get_child(btn, 0)));
    int parenIndex = fullText.indexOf(" (");
    if (parenIndex >= 0) {
        selected_ssid = fullText.substring(0, parenIndex);
    } else {
        selected_ssid = fullText;
    }

    // Create a new screen for password entry
    lv_obj_t * password_window = create_dark_screen();
    load_screen_and_delete_old(password_window);

    lv_obj_set_flex_flow(password_window, LV_FLEX_FLOW_COLUMN);

    // Create a label for the selected SSID
    lv_obj_t * ssid_label = lv_label_create(password_window);
    lv_label_set_text_fmt(ssid_label, "Enter password for SSID: %s", selected_ssid.c_str());
    lv_obj_set_style_text_color(ssid_label, lv_color_hex(0x00e5ff), LV_PART_MAIN);
    lv_obj_align(ssid_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create a text area for password entry
    password_textarea = lv_textarea_create(password_window);
    lv_textarea_set_password_mode(password_textarea, true);
    lv_obj_set_size(password_textarea, 150, 40);
    lv_obj_align(password_textarea, LV_ALIGN_CENTER, 0, 0);

    // Create a keyboard for password entry
    keyboard = lv_keyboard_create(password_window);
    lv_keyboard_set_textarea(keyboard, password_textarea);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Create a button to connect to the WiFi network
    lv_obj_t * connect_btn = lv_btn_create(password_window);
    lv_obj_set_size(connect_btn, 120, 50);
    lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_add_event_cb(connect_btn, [](lv_event_t * e) {
        String password = lv_textarea_get_text(password_textarea);
        connect_to_wifi(selected_ssid, password);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, "Connect");

    // Back → scan window
    create_back_button(password_window, [](lv_event_t * e) { create_wifi_scan_window(); });
}

//----------------------------------------------------
// Connect to a WiFi network
//----------------------------------------------------
void connect_to_wifi(const String& ssid, const String& password) {
    // Create a connection status window
    lv_obj_t * connect_window = create_dark_screen();
    load_screen_and_delete_old(connect_window);
    lv_obj_set_style_pad_all(connect_window, 20, 0);

    // Create status label
    lv_obj_t * status_label = lv_label_create(connect_window);
    lv_label_set_text_fmt(status_label, "Connecting to %s...", ssid.c_str());
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xcccccc), LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);

    // Let the screen update
    lv_timer_handler();

    Serial.print("Connecting to ");
    Serial.println(ssid);

    // Enable persistence and auto-reconnect before attempting connection
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);

    // Ensure WiFi is in the correct mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.printf("Attempting to connect to SSID: '%s'\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    const int maxAttempts = 20;
    bool connected = false;
    String error_msg = "";

    while (attempts < maxAttempts) {
        wl_status_t status = WiFi.status();
        delay(500);
        status = WiFi.status();
        
        switch (status) {
            case WL_CONNECTED:
                connected = true;
                goto connection_complete;
            case WL_NO_SSID_AVAIL:
                delay(1000);
                if (WiFi.status() == WL_CONNECTED) {
                    connected = true;
                    goto connection_complete;
                }
                error_msg = "Network not found";
                goto connection_complete;
            case WL_CONNECT_FAILED:
                delay(1000);
                if (WiFi.status() == WL_CONNECTED) {
                    connected = true;
                    goto connection_complete;
                }
                error_msg = "Connection failed - check password";
                goto connection_complete;
            case WL_DISCONNECTED:
            case WL_IDLE_STATUS:
                break;
            default:
                delay(1000);
                if (WiFi.status() == WL_CONNECTED) {
                    connected = true;
                    goto connection_complete;
                }
                error_msg = "Unknown error: " + String(status);
                goto connection_complete;
        }

        delay(1000);
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

        lv_label_set_text_fmt(status_label, "Connected to %s\nIP: %s", 
            ssid.c_str(), WiFi.localIP().toString().c_str());
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00c853), LV_PART_MAIN);
    } else {
        Serial.print("\nFailed to connect: ");
        Serial.println(error_msg);
        setSolidRed();

        lv_label_set_text_fmt(status_label, "Failed to connect to %s\n%s", 
            ssid.c_str(), error_msg.c_str());
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xff4444), LV_PART_MAIN);
    }

    // Back → WiFi main window
    create_back_button(connect_window, [](lv_event_t * e) { create_wifi_window(); });
}
