#pragma once

// Feature flags — toggle at compile time
// Set to 1 to enable, 0 to disable

// Skip boot checks (WiFi, NTP, registration) — shows all [OK] immediately
#define FF_SKIP_BOOT_CHECKS  1

// Testing mode — enables debug output and test features
#define FF_TESTING           0

// #define FF_SERIAL_TEST  // Uncomment or add -DFF_SERIAL_TEST for test builds

// #define FF_BLE_NO_AUTH  // Disable BLE auth tag verification for testing
