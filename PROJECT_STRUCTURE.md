# Project Structure — BSidesKC 2026 Badge

## Source Layout

```
src/
├── main.cpp                    # Entry point, boot sequence, LVGL init
├── Game/                       # Lunar Lander game engine
│   ├── LunarPhysics.cpp        # Lander physics (gravity, thrust, rotation)
│   ├── LunarTerrain.cpp        # Terrain generation and collision detection
│   ├── LunarState.cpp          # Game state machine, menus, mode select, difficulty
│   ├── LunarRenderer.cpp       # LVGL canvas rendering (terrain, lander, HUD)
│   ├── LunarInput.cpp          # Touch zone input handling
│   ├── LunarAudio.cpp          # Buzzer sounds and LED effects
│   ├── LunarNet.cpp            # WebSocket client for online/multiplayer
│   ├── LunarLobby.cpp          # Room browser, create/join UI
│   └── LunarScoreboard.cpp     # Local high score storage (SD card)
├── QA/                         # Badge app features
│   ├── Menu.cpp                # Main menu, system submenu, window management
│   ├── Menu_Wifi.cpp           # WiFi scan, connect, password entry
│   ├── schedule.cpp            # Conference schedule display
│   ├── Bling.cpp               # NeoPixel LED animations
│   └── ota.cpp                 # Over-the-air firmware updates
└── Hardware/                   # Hardware drivers
    ├── Screen_Module.cpp       # Display init, touch, boot screen
    ├── NeoPixelControl.cpp     # WS2812B LED control
    ├── BadgeRegistration.cpp   # Server registration and check-in
    ├── WiFi_Module.cpp         # WiFi scanning
    └── Status_LED.cpp          # Status indicator LED
```

## Headers

```
include/
├── FeatureFlags.h              # Compile-time toggles (FF_SKIP_BOOT_CHECKS, FF_TESTING)
├── Includes.h                  # Common includes
├── pins.h                      # GPIO pin assignments
├── Game/                       # Game engine headers
│   ├── LunarConfig.h           # Game constants (physics, world size, scoring)
│   ├── LunarPhysics.h          # Lander struct and physics API
│   ├── LunarTerrain.h          # Terrain struct and collision API
│   ├── LunarState.h            # GameState, GamePhase, GameMode enums
│   ├── LunarRenderer.h         # Camera, coordinate transforms
│   ├── LunarInput.h            # InputState struct
│   ├── LunarAudio.h            # Audio/LED function stubs
│   ├── LunarNet.h              # Network client API
│   ├── LunarLobby.h            # Lobby UI API
│   └── LunarScoreboard.h       # Scoreboard struct and API
├── QA/                         # App feature headers
└── Hardware/                   # Hardware driver headers
```

## Other Directories

```
sim/                            # Desktop SDL2 simulator
├── sim_main.cpp                # Standalone game with SDL2 rendering
└── Makefile                    # Build: make && ./lunarlander_sim

test/                           # Native unit tests (31 tests, 5 suites)
├── test_physics/
├── test_terrain/
├── test_collision/
├── test_scoring/
└── test_game_state/

mockups/                        # UI design mockups
└── badge-mockups.html          # All screens at 2× scale
```

## Key Files

| File | Purpose |
|------|---------|
| `platformio.ini` | Build config, pin defines, library deps |
| `include/FeatureFlags.h` | Feature toggles for dev/production |
| `include/Game/LunarConfig.h` | All game constants (physics, scoring, world size) |
| `include/pins.h` | Hardware GPIO assignments |
| `include/lv_conf.h` | LVGL configuration |
| `PORTING_PLAN.md` | Development plan and issue tracking |
