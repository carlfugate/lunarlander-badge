# Project Structure and Feature Map

## Main Folders

- **src/**: Main source code
  - **main.cpp**: Entry point, system setup, main loop
  - **QA/**: Application features and UI logic
  - **Hardware/**: Hardware abstraction and drivers
- **include/**: Public headers (mirrors src/)
- **lib/**: External libraries (e.g., Adafruit_MAX1704X)
- **.pio/**: PlatformIO build output
- **README.md**: Project documentation
- **platformio.ini**: PlatformIO build/configuration
- **schedule.json**: Example schedule data
- **write_version.py**: Versioning utility script

## Features & Module Locations

- **Menu/UI**: `src/QA/Menu.cpp`, `include/QA/Menu.h`
- **Bling (LED effects)**: `src/QA/Bling.cpp`, `include/QA/Bling.hpp`
- **Schedule**: `src/QA/schedule.cpp`, `include/QA/schedule.h`, `schedule.json`
- **OTA Updates**: `src/QA/ota.cpp`, `include/QA/ota.hpp`
- **Wi-Fi Config**: `src/QA/Menu_Wifi.cpp`, `src/QA/WiFi_Settings.cpp`, `include/QA/WiFi_Settings.h`
- **Diagnostics**: `src/QA/Diagnostics.cpp`, `include/QA/Diagnostics.h`
- **NTP Time Sync**: `src/QA/ntp_sync.cpp`, `include/QA/ntp_sync.h`
- **QA Test Sequence**: `src/QA/QA_Test_Sequence.cpp`, `include/QA/QA_Test_Sequence.h`
- **Pirate Ship Animation**: `src/QA/PirateShipAnimation.cpp`, `include/QA/PirateShipAnimation.h`
- **Hardware Drivers**: `src/Hardware/`, `include/Hardware/`
  - NeoPixel: `NeoPixelControl.cpp/h`
  - Badge Registration: `BadgeRegistration.cpp/h`
  - Screen: `Screen_Module.cpp/h`
  - Rotary Encoder: `RotaryEncoder_Module.cpp/h`
  - Status LED: `Status_LED.cpp/h`
  - WiFi Module: `WiFi_Module.cpp/h`

## Configuration & Custom Settings

- **Wi-Fi Networks**: `src/QA/WiFi_Settings.cpp` (array at top), SD card JSON, or via menu
- **OTA Server/Version/Code Name**: `include/QA/ota.hpp`
- **Pin Assignments**: `include/pins.h`, some in `platformio.ini` build flags
- **Display/Touch/Fonts**: `platformio.ini` (build_flags)
- **Schedule Source**: `schedule.json` (SD card or API)
- **Battery Gauge**: `lib/Adafruit_MAX1704X/`

## How to Extend

- Add new menu items: `src/QA/Menu.cpp`
- Add new hardware: `src/Hardware/` and `include/Hardware/`
- Add new features: Follow the pattern in `src/QA/` and `include/QA/`

## Notes
- All persistent settings (WiFi, registration, etc.) are saved to SD card or NVS.
- See `README.md` for more usage and troubleshooting info.

---
This document provides a high-level map of where to find and modify features, configuration, and custom settings in the project.
