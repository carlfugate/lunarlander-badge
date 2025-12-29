#include "QA/WiFi_Settings.h"
#include "Includes.h"
#include <SD.h>
#include <ArduinoJson.h>

// List of Wi-Fi networks (add more networks as needed)
WiFiNetwork wifiNetworks[] = {
    {"BenNet", "River11111"},
    {"BPLabs", "11111"}
};

// Number of networks in the list
const int numNetworks = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// Function to fade out the green LED after Wi-Fi connection
void fadeOutGreen() {
    for (int brightness = 255; brightness >= 0; brightness -= 10) { // Faster fade-out
        statusLED.setPixelColor(0, statusLED.Color(0, brightness, 0)); // Gradual green fade
        statusLED.show();
        delay(100); // Reduced delay for faster fade-out
    }
    clearLED();  // Turn off the LED after fading
}

// Ensure credentials are saved to SD card
void saveWiFiCredentials(const char* ssid, const char* password) {
    // No-op: NVS saving removed
    Serial.println("saveWiFiCredentials is now a no-op (NVS saving removed).");
}

void saveWiFiCredentialsToSD(const char* ssid, const char* password) {
    DynamicJsonDocument doc(256);
    doc["ssid"] = ssid;
    doc["password"] = password;
    File file = SD.open("/ManualWifiConnection.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    serializeJson(doc, file);
    file.close();
    Serial.println("Wi-Fi credentials saved to SD card.");
}


bool loadWiFiCredentialsFromSD(String& ssid, String& password) {
    File file = SD.open("/ManualWifiConnection.json");
    if (!file) {
        Serial.println("No manual Wi-Fi credentials file found.");
        return false;
    }
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        Serial.println("Failed to parse Wi-Fi credentials JSON.");
        return false;
    }
    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    return true;
}

// Try hardcoded WiFi credentials first, then SD card
void connectToWiFi() {
    Serial.println("Setting up Wi-Fi (hardcoded first, then SD card)...");
    // Try hardcoded list
    for (int i = 0; i < numNetworks; ++i) {
        Serial.print("Trying hardcoded network: ");
        Serial.println(wifiNetworks[i].ssid);
        WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
            Serial.println("\nConnected to hardcoded Wi-Fi!");
            Serial.print("Connected to: ");
            Serial.println(wifiNetworks[i].ssid);
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            setSolidGreen();
            fadeOutGreen();
            return;
        }
        Serial.println("\nFailed to connect to hardcoded Wi-Fi.");
    }
    // Try SD card credentials
    String sd_ssid, sd_password;
    if (loadWiFiCredentialsFromSD(sd_ssid, sd_password)) {
        Serial.println("Connecting using SD card credentials...");
        WiFi.begin(sd_ssid.c_str(), sd_password.c_str());
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
            Serial.println("\nConnected to SD card Wi-Fi!");
            Serial.print("Connected to: ");
            Serial.println(sd_ssid);
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            setSolidGreen();
            fadeOutGreen();
            return;
        }
        Serial.println("\nFailed to connect to SD card Wi-Fi.");
    } else {
        Serial.println("No SD card Wi-Fi credentials found. Unable to connect.");
    }
    setSolidRed();
    Serial.println("Unable to connect to any configured networks. Continuing without WiFi.");
}

// Function to display saved Wi-Fi credentials
void displaySavedWiFiCredentials() {
    Serial.println("Checking saved Wi-Fi credentials...");

    if (WiFi.SSID() != "") {
        Serial.print("Saved SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("Saved Password: ");
        // Note: WiFi library does not provide a direct way to retrieve the saved password.
        // This is a placeholder to indicate where the password would be displayed if retrievable.
        Serial.println("[Password is saved but not retrievable via API]");
    } else {
        Serial.println("No Wi-Fi credentials saved.");
    }
}

// Returns true if WiFi is connected after attempting connection
bool connectToWiFiWithResult() {
    connectToWiFi();
    return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
}
