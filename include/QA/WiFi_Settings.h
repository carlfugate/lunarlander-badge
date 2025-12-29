#pragma once

#include <WiFi.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "../Hardware/Status_LED.h"  // Updated path to Status_LED.h

// Define a struct to hold Wi-Fi credentials
struct WiFiNetwork {
    const char* ssid;
    const char* password;
};

// Extern declarations: these tell the compiler that the definitions are elsewhere.
extern WiFiNetwork wifiNetworks[];
extern const int numNetworks;

// Function to fade out the green LED after Wi-Fi connection

// Function to connect to Wi-Fi with persistence enabled
bool connectToWiFiWithResult();

// Function to save Wi-Fi credentials to NVS
void saveWiFiCredentials(const char* ssid, const char* password);

// Functions to load/save Wi-Fi credentials from/to SD card
bool loadWiFiCredentialsFromSD(String& ssid, String& password);
void saveWiFiCredentialsToSD(const char* ssid, const char* password);
