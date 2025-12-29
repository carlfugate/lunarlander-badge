#pragma once

#include "Includes.h"
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>
#include <TFT_eSPI.h>  // TFT library

#define OTA_SERVER_URL "https://bpotaupdates.s3.us-east-1.amazonaws.com/"
#define OTA_MANIFEST_NAME "version" 
#define OTA_BINARY_NAME "firmware.bin" // Don't make this firmware-version-X.bin, this shouldn't need to be changed.
#include "Hardware/BadgeVersion.h"

class OTA{
    private:
        static TFT_eSPI tft;
        static int isWifiAvailable();

        // Progess indicators
        static void indicate_start();
        static void indicate_wifi_connected();
        static void indicate_manifest_load();
        static void indicate_updating();

        // Error indicators
        static void indicate_error_wifi_unavailable();
        static void indicate_error_wifi_rejected(); // PW Fail or uncommon connection hiccup
        static void indicate_error_download();
        static void indicate_error_latest_version();

    public:
        static void checkOTASync();
        void checkForUpdates();
        bool isUpdateAvailable();
        void showUpdateScreen();  // UI handling function
        int getAvailableVersion();
};
// Returns true if 'online' version is newer than 'local' version (semantic versioning)
bool isVersionNewer(const String& online, const String& local);