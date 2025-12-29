#include <WiFi.h>
#include "Hardware/WiFi_Module.h"
#include "Hardware/Status_LED.h"  // Include to control status LED
#include "QA/WiFi_Settings.h" // Include to access saveWiFiCredentials

// Function to scan Wi-Fi networks and update the status LED
void scanWiFiNetworks() {
    Serial.println("Starting Wi-Fi scan...");

    // Set LED to solid yellow during scanning
    setSolidYellow();

    // Disconnect and reset Wi-Fi settings
    WiFi.disconnect(true);  // Clears saved credentials
    WiFi.mode(WIFI_STA);
    delay(100);

    int numNetworks = WiFi.scanNetworks();

    if (numNetworks <= 0) { // Adjusted condition to handle both -1 and 0 networks
        Serial.println("No Wi-Fi networks found or unable to start scan.");
        setSolidRed();  // Solid red if no networks found
        return;
    }

    Serial.print("Found ");
    Serial.print(numNetworks);
    Serial.println(" Wi-Fi networks:");

    // Set LED to solid blue to indicate networks were found
    setSolidBlue();

    for (int i = 0; i < numNetworks; ++i) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(") ");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured");
    }
    Serial.println();
}


String getConnectedWiFiSSID() {
    return WiFi.SSID();
}

String getWiFiConnectionStatus() {
    wl_status_t status = WiFi.status();
    // Check if we have an IP address as additional validation of connection
    if (status == WL_CONNECTED && WiFi.localIP() != IPAddress(0,0,0,0)) {
        return "Connected";
    }
    // If we have an IP but status isn't connected, we might still be connected
    if (WiFi.localIP() != IPAddress(0,0,0,0)) {
        return "Connected";
    }
    return "Disconnected";
}

String getWiFiIPAddress() {
    IPAddress ip = WiFi.localIP();
    if (ip != IPAddress(0,0,0,0)) {
        return ip.toString();
    }
    return "N/A";
}

void connectToWiFi(const char* ssid, const char* password) {
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);

    // Set Wi-Fi mode to station
    WiFi.mode(WIFI_STA);

    // Begin Wi-Fi connection with provided credentials
    WiFi.begin(ssid, password);

    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Retry for 20 attempts
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi connected.");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        // Debug message before saving credentials
        Serial.println("About to call saveWiFiCredentials...");

        // Save credentials to NVS for persistence
        saveWiFiCredentials(ssid, password);

        // Debug message after saving credentials
        Serial.println("saveWiFiCredentials called successfully.");
    } else {
        Serial.println("\nFailed to connect to Wi-Fi.");
    }
}