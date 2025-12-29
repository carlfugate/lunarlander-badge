#include "Hardware/BadgeVersion.h"
#include "Hardware/BadgeRegistration.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "QA/ntp_sync.h" // To get the time

// Define the NVS namespace and key for the registration flag
#define NVS_NAMESPACE "badge_setup"
#define NVS_KEY_REGISTERED "registered"
#define NVS_KEY_TIME "reg_time"
#define NVS_KEY_LAST_CHECKIN "last_checkin"
#define NVS_KEY_CODE_VERSION "code_version"
#define NVS_KEY_CODE_NAME "code_name"
#define NVS_KEY_WIFI_SSID "wifi_ssid"

// The API endpoint
const char* api_url = "https://h9espsict2.execute-api.us-east-1.amazonaws.com/register";

extern NTPManager ntpManager; // Access the global NTPManager instance from main.cpp

void handleBadgeRegistration() {
    Serial.println("--- Badge Registration Check ---");
    
    // Check if already registered
    bool alreadyRegistered = isBadgeRegistered();
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        Serial.printf("[BR] Error opening NVS: %s\n", esp_err_to_name(err));
        return;
    }

    // Perform registration or check-in if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[BR] Registering or checking in badge...");

        // Get Badge ID (MAC Address)
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char badge_id[18];
        snprintf(badge_id, sizeof(badge_id), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // Get Timestamps and Info
        String timestamp = ntpManager.getFormattedTimeISO8601();
        String codeVersion = BADGE_VERSION;
        String codeName = BADGE_CODE_NAME;
        String wifiSSID = WiFi.SSID();
        String regTime = getRegistrationTime();
        if (regTime == "Not Found" || regTime == "Error") regTime = timestamp; // fallback for first registration

        // Construct JSON payload (add new fields)
        String jsonPayload = "{\"badge_id\":\"" + String(badge_id) + "\",";
        jsonPayload += "\"registered_time\":\"" + regTime + "\",";
        jsonPayload += "\"last_checkin\":\"" + timestamp + "\",";
        jsonPayload += "\"code_version\":\"" + codeVersion + "\",";
        jsonPayload += "\"code_name\":\"" + codeName + "\",";
        jsonPayload += "\"wifi_ssid\":\"" + wifiSSID + "\"}";

        Serial.println("[BR] JSON Payload:");
        Serial.println(jsonPayload);

        // Make HTTP POST Request
        HTTPClient http;
        http.begin(api_url);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("[BR] Registration/check-in successful. Saving to NVS.");
            nvs_set_i8(my_handle, NVS_KEY_REGISTERED, 1);
            if (!alreadyRegistered) {
                nvs_set_str(my_handle, NVS_KEY_TIME, timestamp.c_str());
            }
            nvs_set_str(my_handle, NVS_KEY_LAST_CHECKIN, timestamp.c_str());
            nvs_set_str(my_handle, NVS_KEY_CODE_VERSION, codeVersion.c_str());
            nvs_set_str(my_handle, NVS_KEY_CODE_NAME, codeName.c_str());
            nvs_set_str(my_handle, NVS_KEY_WIFI_SSID, wifiSSID.c_str());
            nvs_commit(my_handle);
        } else {
            Serial.printf("[BR] Registration/check-in failed. HTTP: %d\n", httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("[BR] No WiFi. Will retry on next boot.");
    }

    nvs_close(my_handle);
}

bool isBadgeRegistered() {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) return false;

    nvs_handle_t my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return false;

    int8_t badge_registered = 0;
    err = nvs_get_i8(my_handle, NVS_KEY_REGISTERED, &badge_registered);
    nvs_close(my_handle);

    return (err == ESP_OK && badge_registered == 1);
}


String getRegistrationTime() {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) return "Error";
    nvs_handle_t my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return "Error";
    size_t required_size;
    err = nvs_get_str(my_handle, NVS_KEY_TIME, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return "Not Found";
    }
    char* reg_time = new char[required_size];
    err = nvs_get_str(my_handle, NVS_KEY_TIME, reg_time, &required_size);
    nvs_close(my_handle);
    String timeStr = (err == ESP_OK) ? String(reg_time) : "Error";
    delete[] reg_time;
    return timeStr;
}

String getLastCheckinTime() {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) return "Error";
    nvs_handle_t my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return "Error";
    size_t required_size;
    err = nvs_get_str(my_handle, NVS_KEY_LAST_CHECKIN, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return "Not Found";
    }
    char* checkin_time = new char[required_size];
    err = nvs_get_str(my_handle, NVS_KEY_LAST_CHECKIN, checkin_time, &required_size);
    nvs_close(my_handle);
    String timeStr = (err == ESP_OK) ? String(checkin_time) : "Error";
    delete[] checkin_time;
    return timeStr;
}

String getCodeVersion() {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) return "Error";
    nvs_handle_t my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return "Error";
    size_t required_size;
    err = nvs_get_str(my_handle, NVS_KEY_CODE_VERSION, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return "Not Found";
    }
    char* code_version = new char[required_size];
    err = nvs_get_str(my_handle, NVS_KEY_CODE_VERSION, code_version, &required_size);
    nvs_close(my_handle);
    String verStr = (err == ESP_OK) ? String(code_version) : "Error";
    delete[] code_version;
    return verStr;
}

String getWiFiSSID() {
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) return "Error";
    nvs_handle_t my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return "Error";
    size_t required_size;
    err = nvs_get_str(my_handle, NVS_KEY_WIFI_SSID, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return "Not Found";
    }
    char* ssid = new char[required_size];
    err = nvs_get_str(my_handle, NVS_KEY_WIFI_SSID, ssid, &required_size);
    nvs_close(my_handle);
    String ssidStr = (err == ESP_OK) ? String(ssid) : "Error";
    delete[] ssid;
    return ssidStr;
}

// Like handleBadgeRegistration, but returns HTTP result for UI feedback
int handleBadgeRegistrationWithResult() {
    Serial.println("--- Badge Registration Check (with result) ---");
    int httpResponseCode = -1;
    // Check if already registered
    bool alreadyRegistered = isBadgeRegistered();
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    nvs_handle_t my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        Serial.printf("[BR] Error opening NVS: %s\n", esp_err_to_name(err));
        return -2;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[BR] Registering or checking in badge...");
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char badge_id[18];
        snprintf(badge_id, sizeof(badge_id), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        String timestamp = ntpManager.getFormattedTimeISO8601();
        String codeVersion = BADGE_VERSION;
        String codeName = BADGE_CODE_NAME;
        String wifiSSID = WiFi.SSID();
        String regTime = getRegistrationTime();
        if (regTime == "Not Found" || regTime == "Error") regTime = timestamp; // fallback for first registration
        String jsonPayload = "{\"badge_id\":\"" + String(badge_id) + "\",";
        jsonPayload += "\"registered_time\":\"" + regTime + "\",";
        jsonPayload += "\"last_checkin\":\"" + timestamp + "\",";
        jsonPayload += "\"code_version\":\"" + codeVersion + "\",";
        jsonPayload += "\"code_name\":\"" + codeName + "\",";
        jsonPayload += "\"wifi_ssid\":\"" + wifiSSID + "\"}";
        Serial.println("[BR] JSON Payload:");
        Serial.println(jsonPayload);
        HTTPClient http;
        http.begin(api_url);
        http.addHeader("Content-Type", "application/json");
        httpResponseCode = http.POST(jsonPayload);
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("[BR] Registration/check-in successful. Saving to NVS.");
            nvs_set_i8(my_handle, NVS_KEY_REGISTERED, 1);
            if (!alreadyRegistered) {
                nvs_set_str(my_handle, NVS_KEY_TIME, timestamp.c_str());
            }
            nvs_set_str(my_handle, NVS_KEY_LAST_CHECKIN, timestamp.c_str());
            nvs_set_str(my_handle, NVS_KEY_CODE_VERSION, codeVersion.c_str());
            nvs_set_str(my_handle, NVS_KEY_CODE_NAME, codeName.c_str());
            nvs_set_str(my_handle, NVS_KEY_WIFI_SSID, wifiSSID.c_str());
            nvs_commit(my_handle);
        } else {
            Serial.printf("[BR] Registration/check-in failed. HTTP: %d\n", httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("[BR] No WiFi. Will retry on next boot.");
        httpResponseCode = -3;
    }
    nvs_close(my_handle);

    return httpResponseCode;
}
