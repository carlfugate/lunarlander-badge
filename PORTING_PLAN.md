# Lunar Lander — ESP32 Badge Port

**Repo:** [carlfugate/lunarlander-badge](https://github.com/carlfugate/lunarlander-badge)
**Branch:** `feature/lunar-lander`
**Base firmware:** Forked from [BadgePiratesLLC/QACode_27](https://github.com/BadgePiratesLLC/QACode_27)
**Task tracking:** [GitHub Issues](https://github.com/carlfugate/lunarlander-badge/issues)

---

## Overview

Port the Lunar Lander game to the BSidesKC conference badge (ESP32-S3), delivered in three phases:

1. **Standalone** — Full game running locally, no network required
2. **Spectator** — Badge watches live games via WiFi
3. **Multiplayer** — Badge plays against others via game server
4. **Offline Fallback** — If server is unreachable, multiplayer/spectator gracefully fall back to standalone mode. The badge always has the full physics engine locally.

---

## Hardware

| Resource | Spec | Game Use |
|----------|------|----------|
| MCU | ESP32-S3, 16MB flash, PSRAM | — |
| Display | ILI9341 320×240 landscape, RGB565, ~30 FPS | Game viewport |
| Touch | FT6336U capacitive (LVGL pointer indev) | All game input |
| Enter Button | GPIO 38 | Alt thrust |
| Back Button | GPIO 39 | Exit to menu |
| Buzzer | GPIO 19 | Sound effects |
| NeoPixels | 6× WS2812B GPIO 18 | State FX |
| Status LED | 1× WS2812B GPIO 21 | State indicator |
| LVGL heap | 64KB | UI widgets |
| PSRAM | Available | Canvas buffer (153KB) |
| WiFi | ESP32-S3 built-in | Spectator + multiplayer |
| Font | Montserrat 14 | HUD text |

---

## File Structure

```
include/Game/
    LunarConfig.h        # Constants
    LunarPhysics.h       # Lander struct + physics
    LunarTerrain.h       # Terrain gen + collision
    LunarState.h         # State machine, scoring, entry points
    LunarRenderer.h      # LVGL canvas + camera
    LunarInput.h         # Touch zones + buttons
    LunarAudio.h         # Buzzer + LEDs
    LunarNet.h           # WebSocket client (Phase 2+)
    LunarLobby.h         # Room browser UI (Phase 3)
    LunarScoreboard.h    # Local high score storage

src/Game/
    (matching .cpp files)

sim/
    sim_main.cpp         # SDL2 desktop simulator
    Makefile             # Build with: make && make run

include/
    FeatureFlags.h       # Compile-time feature toggles

mockups/
    badge-mockups.html   # UI design mockups (2x scale)

test/
    test_physics/test_physics.cpp
    test_terrain/test_terrain.cpp
    test_collision/test_collision.cpp
    test_scoring/test_scoring.cpp
    test_game_state/test_game_state.cpp
```

---

## Menu Integration (Redesigned)

The main menu was completely redesigned for BSidesKC 2026 with a dark theme, prioritized button layout, LVGL icons, system sub-menu, and back-button navigation. The original 3-line integration was replaced by a full UI overhaul across `Menu.cpp`, `Includes.h`, and new sub-menu screens.

---

## Phase 1 — Standalone Single-Player

Game runs 100% locally. No WiFi needed. This engine is also the offline fallback for all network modes.

| Issue | Title | Type | Depends On |
|-------|-------|------|------------|
| [#1](https://github.com/carlfugate/lunarlander-badge/issues/1) | LunarConfig.h — Game constants | engine | — |
| [#2](https://github.com/carlfugate/lunarlander-badge/issues/2) | LunarPhysics — Lander struct and physics | engine | #1 |
| [#3](https://github.com/carlfugate/lunarlander-badge/issues/3) | LunarTerrain — Terrain generation and collision | engine | #1, #2 |
| [#4](https://github.com/carlfugate/lunarlander-badge/issues/4) | LunarState — Game state machine and scoring | engine | #1, #2, #3 |
| [#5](https://github.com/carlfugate/lunarlander-badge/issues/5) | Native unit tests | testing | #1–#4 |
| [#6](https://github.com/carlfugate/lunarlander-badge/issues/6) | LunarRenderer — LVGL canvas and camera | rendering | #1, #4 |
| [#7](https://github.com/carlfugate/lunarlander-badge/issues/7) | LunarInput — Touch zone input | input | — |
| [#8](https://github.com/carlfugate/lunarlander-badge/issues/8) | LunarAudio — Buzzer and LED effects | audio | — |
| [#9](https://github.com/carlfugate/lunarlander-badge/issues/9) | Game loop wiring | integration | #4, #6, #7, #8 |
| [#10](https://github.com/carlfugate/lunarlander-badge/issues/10) | Badge menu integration | integration | #9 |
| [#11](https://github.com/carlfugate/lunarlander-badge/issues/11) | Hardware integration testing | testing | #9, #10 |

**Parallelizable:** #6, #7, #8 can be built in parallel after #4 is done. #1→#2→#3→#4 is sequential.

---

## Phase 2 — Spectator Mode

Badge connects to game server via WiFi, watches live games.

| Issue | Title | Type | Depends On |
|-------|-------|------|------------|
| [#12](https://github.com/carlfugate/lunarlander-badge/issues/12) | LunarNet — WebSocket client | network | WiFi (base firmware) |
| [#13](https://github.com/carlfugate/lunarlander-badge/issues/13) | Spectator mode — session browser + live viewing | network | #6, #12 |
| [#14](https://github.com/carlfugate/lunarlander-badge/issues/14) | Spectator hardware testing | testing | #13 |

**Key details:**
- Connects to `/spectate/{sessionId}` (read-only WebSocket)
- Receives telemetry at 30Hz, renders terrain + lander(s)
- Handles both single-player and multiplayer telemetry formats
- Session discovery via REST `GET /rooms`

---

## Phase 3 — Multiplayer

Badge connects as a player with server-authoritative physics.

| Issue | Title | Type | Depends On |
|-------|-------|------|------------|
| [#15](https://github.com/carlfugate/lunarlander-badge/issues/15) | LunarLobby — Room browser, create, join | network | #12 |
| [#16](https://github.com/carlfugate/lunarlander-badge/issues/16) | Multiplayer gameplay — networked play | network | #12, #15, #6, #7 |
| [#17](https://github.com/carlfugate/lunarlander-badge/issues/17) | Multiplayer hardware testing | testing | #16 |

**Key details:**
- Server is authoritative when connected — badge runs local physics as prediction/fallback
- Badge sends input actions (`thrust_on/off`, `rotate_left/right/stop`)
- Receives all players' state in telemetry, renders color-coded landers
- Lobby: browse rooms, create/join, waiting lobby with player list, countdown
- Game over: ranked results

---

## Offline Fallback Strategy

The badge always ships with the full physics engine (Phase 1). Network modes layer on top:

- **Standalone (Phase 1):** Local physics only. Always available.
- **Spectator (Phase 2):** Server sends telemetry, badge renders. If server disconnects mid-game, show "Connection Lost" and return to menu. No local physics needed for spectating.
- **Multiplayer (Phase 3):** Server is authoritative. Badge sends input, receives state. If server is unreachable at game start, offer standalone mode instead. If server disconnects mid-game, show "Connection Lost" and return to menu.
- **Mode selection:** When player taps "Lander" in menu, show mode select: PLAY (standalone, always available) / SPECTATE (grayed out if no WiFi) / MULTIPLAYER (grayed out if no WiFi). WiFi status checked at mode select time.

This means the Phase 1 physics/terrain/collision/scoring code is the foundation for ALL modes — it's never thrown away or bypassed.

---

## Phase 4 — UI/UX Polish (DONE)

| Feature | Status |
|---------|--------|
| BSidesKC 2026 themed boot screen | ✅ Done |
| Dark theme main menu with prioritized layout | ✅ Done |
| System sub-menu for QA tools | ✅ Done |
| Dark theme on all sub-windows | ✅ Done |
| Back button navigation (one level up) | ✅ Done |
| Scrolling mission control ticker | ✅ Done |
| LVGL icons on all buttons | ✅ Done |
| Updated credits with BSidesKC branding | ✅ Done |
| Feature flags (FF_SKIP_BOOT_CHECKS) | ✅ Done |
| LVGL screen memory leak fix | ✅ Done |
| Heap debug logging | ✅ Done |

---

## Game Improvements (DONE)

| Feature | Status |
|---------|--------|
| Fixed-step physics accumulator (real-time on hardware) | ✅ Done |
| Online/offline mode toggle on difficulty select | ✅ Done |
| New touch controls (thrust left, rotate right) | ✅ Done |
| Speed HUD with direction color (red=falling, green=rising) | ✅ Done |
| Local scoreboard — top 5, persisted to SD | ✅ Done |
| GameMode enum (offline, online solo, multiplayer, spectate) | ✅ Done |
| Desktop SDL2 simulator | ✅ Done |

---

## Touch Controls

```
┌──────────┬──────────┐
│          │ Rotate   │
│  THRUST  │  Left    │
│  (left   ├──────────┤
│   half)  │ Rotate   │
│          │  Right   │
└──────────┴──────────┘
```

Left half (x < 160) = thrust. Top-right (x >= 160, y < 120) = rotate left. Bottom-right (x >= 160, y >= 120) = rotate right.

Enter button (GPIO 38) = alt thrust. Back button (GPIO 39) = exit.

---

## Game Constants (from Python server)

| Constant | Value |
|----------|-------|
| GRAVITY | 1.62 m/s² |
| THRUST_POWER | 8.0 m/s² |
| ROTATION_SPEED | 3.0 rad/s |
| FUEL_CONSUMPTION | 10.0 units/s |
| INITIAL_FUEL | 1000.0 |
| MAX_LANDING_SPEED | 5.0 m/s |
| MAX_LANDING_ANGLE | 0.3 rad (~17°) |
| WORLD_W × WORLD_H | 1200 × 800 |
| START_X, START_Y | 600, 100 |

---

## Memory Strategy

Canvas buffer = 320×240×2 = 153KB → PSRAM via `heap_caps_malloc(MALLOC_CAP_SPIRAM)`.
Fallback: `malloc` if PSRAM unavailable.

---

## Testing

### Native Unit Tests (Phase 1)

`pio test -e native` — 5 suites, ~30 tests total. See [#5](https://github.com/carlfugate/lunarlander-badge/issues/5).

### Hardware Tests

Manual checklists per phase:
- Phase 1: [#11](https://github.com/carlfugate/lunarlander-badge/issues/11)
- Phase 2: [#14](https://github.com/carlfugate/lunarlander-badge/issues/14)
- Phase 3: [#17](https://github.com/carlfugate/lunarlander-badge/issues/17)

### Performance Targets

- Physics tick < 1ms
- Frame render < 30ms
- Canvas in PSRAM
- Game heap overhead < 10KB (excl. canvas)

---

## Open Decisions

1. ~~**Canvas resolution**~~ — RESOLVED: Full 320×240 in PSRAM, fallback to `malloc` if PSRAM unavailable.
2. ~~**Touch feel**~~ — RESOLVED: Left/right split layout implemented (thrust left half, rotate right half).
3. ~~**High scores**~~ — RESOLVED: SD card (`/lander_scores.dat`), top 5 scores.
4. **Larger font** — Enable Montserrat 20/26 in lv_conf.h for score display (~5-10KB flash).
5. ~~**Server URL**~~ — RESOLVED: Defaults in `LunarNet.h`, overridable via build flags (`-DLN_SERVER_HOST`).

## Recent Changes

### Visual Polish
- Terrain fill below surface (dark green, direct RGB565 buffer write for performance)
- Bright 3×3 center dot on lander for visibility at 0.267× scale
- "Connecting..." status during online solo PHASE_WAITING

### Online Solo Mode (MODE_ONLINE_SOLO)
- Connects to server `/ws` endpoint, sends `start` message with difficulty
- Sends input deltas (thrust_on/off, rotate_left/right/stop) to server
- Polls server for authoritative state (telemetry, game_over)
- 5-second timeout falls back to offline mode if server unreachable
- Disconnects on game over and cleanup

### Bug Fixes
- #19: WiFi scan page overlap (smaller buttons, tighter padding, scroll)
- #20: Credits screen overlap (tightened y positions, body anchored to TOP_MID)
- #21: Bling + Lander crash (stop bling ticker before entering game)
