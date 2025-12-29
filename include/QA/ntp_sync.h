#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class NTPManager {
private:
    const char* ntpServer;
    const char* timezone;

public:
    NTPManager(const char* ntpServer = "pool.ntp.org", const char* timezone = "CST6CDT,M3.2.0/2,M11.1.0/2");
    bool init();
    String getFormattedTime();
    String getFormattedTimeISO8601(); // New function
};

