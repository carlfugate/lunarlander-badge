# Copilot Instructions for BasicCodeForQA

## Project Overview
- **Target:** ESP32-S3 badge with LVGL UI, RGB LEDs, buzzer, SD card, battery gauge, and Wi-Fi.
- **Architecture:** Modular C++ codebase using PlatformIO/Arduino. UI, hardware drivers, and logic are separated for clarity and extensibility.
- **Key Directories:**
  - `src/QA/`: UI logic, features, and menu windows (e.g., Bling, OTA, Schedule, Diagnostics)
  - `src/Hardware/`: Hardware abstraction (NeoPixel, display, rotary encoder, etc.)
  - `include/`: Public headers, mirrors `src/` structure
  - `lib/Adafruit_MAX1704X/`: Battery gauge library

## Build & Flash Workflow
- Use **PlatformIO** (VSCode extension or CLI):
  - Build: `PlatformIO: Build` or `pio run`
  - Upload: `PlatformIO: Upload` or `pio run -t upload`
  - Serial monitor: `PlatformIO: Monitor` or `pio device monitor`
- Board: `esp32-s3-devkitc-1` (see `platformio.ini`)
- All build flags and dependencies are managed in `platformio.ini`.

## Key Patterns & Conventions
- **Menu/UI:**
  - Add new menu items in `src/QA/Menu.cpp`.
  - Each feature window is a function in `Menu.cpp` or its own file in `src/QA/`.
  - Use LVGL for UI elements; follow patterns in existing menu/window code.
- **Hardware Abstraction:**
  - Drivers in `src/Hardware/`, headers in `include/Hardware/`.
  - Pin assignments in `include/pins.h` and `platformio.ini` build flags.
- **Persistent Settings:**
  - Wi-Fi, registration, and other settings are saved to SD card or NVS.
  - Wi-Fi config logic in `src/QA/WiFi_Settings.cpp` and menu in `Menu_Wifi.cpp`.
- **Schedule:**
  - Loads from `schedule.json` (SD card) or API; see `src/QA/schedule.cpp`.
- **OTA Updates:**
  - Logic in `src/QA/ota.cpp`/`ota.hpp`, config in `include/QA/ota.hpp`.
- **Battery Gauge:**
  - Uses Adafruit MAX1704X library in `lib/Adafruit_MAX1704X/`.

## Integration & Extension
- Add new features by following the structure in `src/QA/` and `include/QA/`.
- Add new hardware by creating drivers in `src/Hardware/` and headers in `include/Hardware/`.
- Reference `PROJECT_STRUCTURE.md` and `README.md` for feature locations and extension points.

## Troubleshooting & Debugging
- Use serial output for error/debug messages.
- Common issues and solutions are documented in `README.md` (SD card, OTA, touch, LEDs).
- Pinout and hardware connections are in `include/pins.h`.

## Examples
- To add a new menu item: Edit `src/QA/Menu.cpp`, create a new window function, and add to the menu list.
- To add a new hardware driver: Implement in `src/Hardware/`, add header in `include/Hardware/`, and reference in main logic as needed.

---
For more details, see `README.md` and `PROJECT_STRUCTURE.md`.
