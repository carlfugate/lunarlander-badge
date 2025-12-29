#include "QA/ntp_sync.h"
#include <WiFi.h>

NTPManager::NTPManager(const char* ntpServer, const char* timezone)
    : ntpServer(ntpServer), timezone(timezone) {}

bool NTPManager::init() {
    // Check WiFi connection first
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️  WiFi not connected - skipping NTP sync");
        return false;
    }

    configTzTime(timezone, ntpServer);

    struct tm timeinfo;
    int retries = 0;
    const int maxRetries = 10;

    while (!getLocalTime(&timeinfo) && retries < maxRetries) {
        Serial.println("Waiting for NTP time sync...");
        delay(500);
        retries++;
    }

    if (retries == maxRetries) {
        Serial.println("⚠️  Failed to sync time via NTP - continuing without time sync");
        return false;
    }

    Serial.println("✅ NTP time synchronized");
    Serial.println(&timeinfo, "Time is: %A, %B %d %Y %H:%M:%S");
    return true;
}

String NTPManager::getFormattedTime() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buffer[30];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return String(buffer);
    }
    return "Time Not Set";
}

String NTPManager::getFormattedTimeISO8601() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buffer[30];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
        return String(buffer);
    }
    return ""; // Return empty string if time is not set
}
