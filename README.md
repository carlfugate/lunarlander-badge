# BSidesKC 2026 Badge

Conference badge firmware for [BSidesKC 2026](https://bsideskc.org) (April 25, Kansas City). Built on ESP32-S3 with a 320×240 touchscreen, featuring the Lunar Lander game, conference schedule, check-in, and more.

Forked from [BadgePiratesLLC/QACode_27](https://github.com/BadgePiratesLLC/QACode_27).

## Features

- **Lunar Lander Game** — Full physics engine with 3 difficulty levels, local scoreboard, online/offline modes, spectator mode, and multiplayer
- **Conference Schedule** — View talks and events
- **Badge Check-In** — Register attendance at the conference
- **Bling** — Animated NeoPixel LED patterns
- **WiFi Management** — Scan, connect, credentials saved to SD
- **OTA Updates** — Over-the-air firmware updates
- **System Tools** — Battery, buzzer, SD card browser, system info

## Hardware

| Component | Spec | Pin(s) |
|-----------|------|--------|
| MCU | ESP32-S3, 16MB flash | — |
| Display | ILI9341 320×240 SPI | MOSI:11, SCLK:12, CS:10, DC:5, RST:4, BL:6 |
| Touch | FT6336U capacitive I2C | SDA:8, SCL:9, RST:3, INT:7 |
| NeoPixels | 6× WS2812B | GPIO 18 |
| Status LED | 1× WS2812B | GPIO 21 |
| Buzzer | Piezo | GPIO 19 |
| SD Card | SPI | MOSI:35, SCK:36, MISO:37, CS:47 |
| Buttons | Boot(0), Enter(38), Back(39) | — |
| Battery | MAX17048 I2C gauge | Shared I2C bus |

## Quick Start

```bash
# Build and flash
pio run -t upload

# Monitor serial output
pio device monitor

# Run unit tests
pio test -e native

# Build and run desktop simulator
cd sim && make && ./lunarlander_sim
```

## Lunar Lander Game

Physics matched to the [web version](https://github.com/carlfugate/lunarlander): gravity=1.62, thrust=8.0, max landing speed=5.0.

**Controls (touchscreen):**
- Left half: Thrust
- Top-right: Rotate left
- Bottom-right: Rotate right
- Back button: Exit to menu

**Modes:**
- Offline single-player (local physics)
- Online single-player (server-authoritative, leaderboard) — *in progress*
- Multiplayer (rooms via game server)
- Spectator (watch live games)

**Desktop Simulator:** `cd sim && make && ./lunarlander_sim [difficulty]` (0=easy, 1=medium, 2=hard). Requires SDL2 and SDL2_ttf.

## Project Structure

```
src/Game/          — Lunar Lander engine (physics, terrain, renderer, input, audio, net, lobby, scoreboard)
src/QA/            — Badge app features (menu, WiFi, schedule, bling, OTA)
src/Hardware/      — Hardware drivers (screen, touch, LEDs, registration)
include/           — Headers + FeatureFlags.h
sim/               — SDL2 desktop simulator
test/              — Native unit tests (31 tests)
mockups/           — UI design mockups
```

## Feature Flags

Edit `include/FeatureFlags.h`:
- `FF_SKIP_BOOT_CHECKS` — Skip WiFi/NTP/registration during boot (fast dev iteration)
- `FF_TESTING` — Enable debug features

## Credits

- **Badge Hardware:** [BadgePirates / BPLabs](https://bplabs.tech)
- **UI Framework:** [LVGL](https://lvgl.io/)
- **Build System:** [PlatformIO](https://platformio.org/)
- **Conference:** [BSidesKC](https://bsideskc.org)
