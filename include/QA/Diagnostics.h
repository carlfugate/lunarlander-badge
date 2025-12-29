#pragma once

#include <SD.h>

// Initialize diagnostics
bool initDiagnostics();

#ifdef DIAGNOSTIC_LOGGING  // Only include these if DIAGNOSTIC_LOGGING is enabled
    void logSystemResources();      // Logs memory usage and uptime
    void logWiFiDiagnostics();      // Logs Wi-Fi signal strength and reconnect attempts
    void logSDCardDiagnostics();    // Logs SD card usage and checks for file system errors
    void runDiagnostics();          // Main function to run all diagnostics and save to SD card
#endif
