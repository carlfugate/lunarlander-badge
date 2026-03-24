# Copilot Instructions — BSidesKC 2026 Badge

## Project Overview
- **Target:** ESP32-S3 conference badge for BSidesKC 2026
- **Display:** ILI9341 320×240 with FT6336U capacitive touch
- **UI:** LVGL 9.2, dark theme with cyan/green accents
- **Game:** Lunar Lander with full physics engine, scoreboard, online/offline modes
- **Architecture:** Modular C++ with PlatformIO/Arduino framework

## Key Directories
- `src/Game/` — Lunar Lander engine (physics, terrain, renderer, input, audio, networking, scoreboard)
- `src/QA/` — Badge app features (menu, WiFi, schedule, bling, OTA)
- `src/Hardware/` — Hardware drivers (screen, touch, LEDs, registration)
- `include/` — Headers, feature flags, game config, pin assignments
- `sim/` — SDL2 desktop simulator
- `test/` — Native unit tests (31 tests)

## Build & Test
- Build: `pio run`
- Flash: `pio run -t upload` (460800 baud)
- Test: `pio test -e native` (31 unit tests)
- Simulator: `cd sim && make && ./lunarlander_sim`
- Monitor: `pio device monitor`

## Conventions
- Dark theme: bg `#0a0a0f`, text `#cccccc`, accent cyan `#00e5ff`, green `#00c853`
- All screens use `load_screen_and_delete_old()` to prevent LVGL memory leaks
- Back buttons navigate one level up, not to home
- Game code guarded with `#ifndef NATIVE_TEST` for platform-specific sections
- Feature flags in `include/FeatureFlags.h` for compile-time toggles
- Pin assignments in `include/pins.h` and `platformio.ini` build flags

## Game Physics (matched to web version)
- Gravity: 1.62, Thrust: 8.0, Max landing speed: 5.0
- World: 1200×800 scaled to 320×240 (full world always visible)
- Fixed-step physics at 60Hz with time accumulator for frame-rate independence

## Adding Features
- New menu items: Edit `src/QA/Menu.cpp`, add to `display_main_menu_buttons()` or `create_system_submenu()`
- New game features: Add to `src/Game/`, header in `include/Game/`
- New hardware: Driver in `src/Hardware/`, header in `include/Hardware/`
