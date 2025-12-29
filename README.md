
# BasicCodeForQA

Welcome! This is the main entry point for users and developers. For a technical deep dive, see:
- [Project Structure and Feature Map](PROJECT_STRUCTURE.md): Codebase layout, feature locations, and extension points for developers.
- [Project Summary](project_summary.md): Recent changes, refactoring, and code cleanup history.

# Hardware Used

- ESP32-S3 microcontroller (main board)
- TFT LCD display (with touch, LVGL compatible)
- WS2812/NeoPixel RGB LEDs (for Bling and RGB features)
- Standard LEDs (for LED on/off control)
- Buzzer (for tone/frequency slider)
- SD card (for Wi-Fi config, schedule, and data storage)
- MAX17048 battery fuel gauge (for battery status/voltage)
- Wi-Fi (built-in ESP32)
- (Optional) I2C touch controller (if using touch display)

# Features

- Modern LVGL-based UI with gradient/shadow buttons and white backgrounds
- Main menu with quick access to all features
- RGB/NeoPixel control (Red, Green, Blue, Off)
- Bling menu with animated LED patterns (Rainbow, Police, All Blink, Chase, Random, Off)
- LED on/off control
- Buzzer frequency slider
- Battery status and voltage display with color-coded gauge
- Wi-Fi configuration and persistent storage (SD card)
- OTA update support with version check and progress display
- SD card file browser
- System info (heap, MAC address, version)
- Credits/about window
- Schedule display (list/card view, loads from SD or API, error handling)
- Robust error feedback (serial and on-screen)
- Modular code structure for easy feature addition
- Consistent back button and navigation

# File Structure

- `src/` — Main source code directory
  - `main.cpp` — Entry point, system setup and main loop
  - `QA/` — Badge app features and UI logic
    - `Menu.cpp` / `Menu.h` — Main menu, navigation, and window creation
    - `Bling.cpp` / `Bling.hpp` — Bling (RGB/LED effects) menu and animation logic
    - `schedule.cpp` / `schedule.h` — Schedule loading, display, and error handling
    - `ota.cpp` / `ota.hpp` — OTA update logic and UI
    - `Menu_Wifi.cpp` — Wi-Fi configuration window
    - `Diagnostics.cpp` — Diagnostics and error reporting
    - `ntp_sync.cpp` / `ntp_sync.h` — NTP time sync
    - `QA_Test_Sequence.cpp` / `QA_Test_Sequence.h` — Automated QA test logic
    - `PirateShipAnimation.cpp` / `PirateShipAnimation.h` — Fun animation feature
    - `WiFi_Settings.cpp` / `WiFi_Settings.h` — Wi-Fi settings persistence
  - `Hardware/` — Hardware abstraction and drivers
    - `NeoPixelControl.cpp` / `NeoPixelControl.h` — NeoPixel/WS2812 LED control
    - `BadgeRegistration.cpp` / `BadgeRegistration.hpp` — Badge registration logic
    - `Screen_Module.cpp` / `Screen_Module.h` — Display/touch abstraction
    - `RotaryEncoder_Module.cpp` / `RotaryEncoder_Module.h` — Rotary encoder abstraction
    - `Status_LED.cpp` / `Status_LED.h` — Status LED control
    - `WiFi_Module.cpp` / `WiFi_Module.h` — Wi-Fi hardware abstraction
- `include/` — Public headers for all modules (mirrors structure of `src/`)
- `lib/Adafruit_MAX1704X/` — MAX17048 battery gauge library
- `platformio.ini` — PlatformIO project configuration
- `README.md` — Project documentation
- `schedule.json`, `HackerTrackerSchedule.json` — Example schedule data
- `write_version.py` — Versioning utility script

Each module is organized for clarity and modularity. UI, hardware, and logic are separated for easy maintenance and extension. See source files for further details on each feature.

# Getting Started

1. **Prerequisites:**
   - [PlatformIO](https://platformio.org/) (VSCode extension or CLI)
   - Python 3.x (for PlatformIO and utility scripts)
   - USB drivers for ESP32-S3 (if needed)

2. **Build & Flash:**
   - Open this folder in VSCode with PlatformIO installed, or use PlatformIO CLI.
   - Connect your ESP32-S3 board via USB.
   - Run `PlatformIO: Build` and `PlatformIO: Upload` (or `pio run -t upload` in terminal).

3. **First Boot:**
   - The badge will boot to the main menu.
   - Use the touchscreen or physical buttons (if present) to navigate.
   - Configure Wi-Fi via the menu for OTA and schedule features.

# Usage

- **Navigation:**
  - Use the touchscreen or hardware buttons to select menu items.
  - The back button is present on every screen for easy navigation.
- **OTA Updates:**
  - Select 'OTA' from the main menu to check for and apply updates.
- **Wi-Fi:**
  - Configure Wi-Fi from the menu. Credentials are saved to SD card.
- **Schedule:**
  - View the event schedule (from SD or API) in a card/list format.
- **Bling:**
  - Use the Bling menu to activate animated RGB/LED effects.
- **Battery:**
  - View battery percentage and voltage (requires MAX17048).
- **SD Card:**
  - Browse files on the SD card.
- **System Info:**
  - View firmware version, memory, and MAC address.

# Pinout / Hardware Connections

| Function         | ESP32 Pin | Notes                        |
|------------------|-----------|------------------------------|
| NeoPixel Data    | (see pins.h) | WS2812/NeoPixel chain       |
| Buzzer           | (see pins.h) | PWM capable                 |
| SD Card          | (see pins.h) | SPI or SDMMC                |
| MAX17048         | I2C pins  | Battery gauge (optional)     |
| TFT Display      | (see pins.h) | SPI/I2C, touch optional     |
| LEDs             | (see pins.h) | Standard digital outputs    |
| I2C Touch        | I2C pins  | Optional, for touch display  |

See `include/pins.h` for exact pin assignments for your hardware variant.

# Customization & Extending

- Add new menu items by editing `src/QA/Menu.cpp` and creating new window functions.
- Add new hardware drivers in `src/Hardware/` and corresponding headers in `include/Hardware/`.
- UI and logic are modular—follow the pattern in existing files for new features.

# Troubleshooting

- **SD Card not detected:**
  - Check wiring and format (FAT32 recommended).
- **OTA fails:**
  - Ensure Wi-Fi is configured and badge can reach the update server.
- **Touch not working:**
  - Confirm I2C address and wiring for touch controller.
- **No LEDs/Bling:**
  - Check NeoPixel data pin and power.
- **Other issues:**
  - Use serial output for error/debug messages.

# License

This project is provided as-is for educational and demonstration purposes. See `lib/Adafruit_MAX1704X/license.txt` for third-party library licenses.

# Credits / Acknowledgments

- [LVGL](https://lvgl.io/) for the UI library
- [Adafruit](https://adafruit.com/) for MAX17048 and NeoPixel libraries
- [PlatformIO](https://platformio.org/) for build system
- BPLabs and contributors

