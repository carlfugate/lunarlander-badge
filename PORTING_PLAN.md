# Lunar Lander — ESP32 Badge Port

## Repo: [carlfugate/lunarlander-badge](https://github.com/carlfugate/lunarlander-badge)
## Branch: `feature/lunar-lander`
## Base: Forked from [BadgePiratesLLC/QACode_27](https://github.com/BadgePiratesLLC/QACode_27)

---

## Overview

Port the Lunar Lander game to the BSidesKC conference badge (ESP32-S3).
Three modes, delivered incrementally:

1. **Standalone** — Full game running locally on badge, no network
2. **Spectator** — Badge connects to game server, watches live games
3. **Multiplayer** — Badge connects to server as a player, plays against others

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
| PSRAM | Available | Canvas buffer |
| WiFi | ESP32-S3 built-in | Spectator + multiplayer |
| Font | Montserrat 14 | HUD text |

---

## File Structure

```
include/Game/
    LunarConfig.h        # Constants
    LunarPhysics.h       # Lander struct + physics update
    LunarTerrain.h       # Terrain generation + collision
    LunarState.h         # State machine, scoring, entry points
    LunarRenderer.h      # LVGL canvas drawing + camera
    LunarInput.h         # Touch zones + button reads
    LunarAudio.h         # Buzzer tones + LED effects
    LunarNet.h           # WebSocket client (spectator + multiplayer)
    LunarLobby.h         # Room browser + join/create UI

src/Game/
    LunarPhysics.cpp
    LunarTerrain.cpp
    LunarState.cpp
    LunarRenderer.cpp
    LunarInput.cpp
    LunarAudio.cpp
    LunarNet.cpp
    LunarLobby.cpp

test/
    test_physics/test_physics.cpp
    test_terrain/test_terrain.cpp
    test_collision/test_collision.cpp
    test_scoring/test_scoring.cpp
    test_game_state/test_game_state.cpp
```

---

## Menu Integration (3 changes to existing code)

1. **Menu.cpp** → `display_main_menu_buttons()`: add `"Lander"` to `buttons[]`
2. **Menu.cpp** → `button_event_handler()`: add `else if` for `"Lander"` → `lunar_lander_start()`
3. **Includes.h**: add `#include "Game/LunarState.h"`

---

## Memory Strategy

Canvas buffer (320×240×2 = 153KB) allocated in PSRAM.
Fallback to 160×120 (~38KB) if PSRAM unavailable.

---

## Phases

### Phase 1 — Standalone Single-Player

Game runs 100% locally. No WiFi needed.

**1.1 Core Engine (pure C++, testable on desktop)**

- `LunarConfig.h` — all constants from Python server
- `LunarPhysics.h/.cpp` — Lander struct, Euler integration per tick
  - rotation ± ROTATION_SPEED × dt, clamp ±π
  - thrust vector: vx += sin(rot)×THRUST×dt, vy -= cos(rot)×THRUST×dt
  - gravity: vy += 1.62 × dt
  - fuel burn: fuel -= CONSUMPTION × dt
- `LunarTerrain.h/.cpp` — random walk terrain, one flat landing zone
  - difficulty controls: step (50/40/30), variation (±20/30/50), zone width (100/80/60)
  - linear interpolation for height_at(x)
  - collision: lander.y >= terrain height → check landing criteria
- `LunarState.h/.cpp` — state machine (MENU→PLAYING→LANDED/CRASHED)
  - scoring: (1000 + fuel_bonus + time_bonus) × difficulty_multiplier

**1.2 Native Tests**

PlatformIO `[env:native]` with Unity test framework.
5 test suites: physics, terrain, collision, scoring, game_state.

**1.3 Rendering**

- LVGL canvas in PSRAM, 320×240 viewport into 1200×800 world
- Camera centers on lander, clamped to world bounds
- Draw: stars → terrain (green line + dark fill + bright zone) → lander (rotated shape) → thrust flame → explosion particles
- HUD: fuel bar, speed, altitude, warnings (LVGL labels overlaid on canvas)

**1.4 Input**

Touch zones:
- Top half → thrust
- Bottom-left → rotate left
- Bottom-right → rotate right
- Enter button (GPIO 38) → alt thrust
- Back button (GPIO 39) → exit

**1.5 Audio & LEDs**

- Buzzer: thrust tone, landing jingle, crash tone, low fuel beep
- NeoPixels: orange (thrust), green (landed), red (crashed)

**1.6 UI Flow**

- Menu button → difficulty select (EASY/MEDIUM/HARD) → gameplay → game over (score + AGAIN/MENU)
- Game loop via `lv_timer_create()` at 16ms (~60Hz physics, ~30Hz render)

---

### Phase 2 — Spectator Mode

Badge connects to game server via WiFi, watches live games read-only.

**2.1 WebSocket Client**

- `LunarNet.h/.cpp` — connects to server's `/spectate/{sessionId}` endpoint
- Receives `init` (terrain + lander state) and `telemetry` at 30Hz
- Handles both single-player format (`lander: {}`) and multiplayer format (`players: {}`)
- Receives `game_over` with results

**2.2 Session Discovery**

- REST `GET /rooms` to list active sessions
- Simple LVGL list UI to pick a session to watch

**2.3 Spectator Renderer**

- Reuses standalone renderer for terrain + lander drawing
- Multiplayer: draws multiple landers with different colors
- HUD shows spectator-specific info (player names, spectator count)
- No input sent to server (read-only connection)

---

### Phase 3 — Multiplayer

Badge connects as a player. Full game interaction via server.

**3.1 Lobby**

- `LunarLobby.h/.cpp` — LVGL UI for room browser
- REST `GET /rooms` → list rooms
- Create room: WS `create_room` message → waiting lobby
- Join room: WS `join_room` message → waiting lobby
- Waiting lobby: player list, "Start Game" button (creator), countdown

**3.2 Networked Play**

- `LunarNet.cpp` extended for player mode
- Connect to `/ws`, send `start` or `create_room`/`join_room`
- Send input: `thrust_on`, `thrust_off`, `rotate_left`, `rotate_right`, `rotate_stop`
- Receive `telemetry` at 60Hz with server-authoritative lander state
- No local physics — server is authoritative, badge just renders
- Touch input mapped to the same action strings

**3.3 Multiplayer Renderer**

- Draws all players from `players` dict in telemetry
- Color-coded landers per player
- Game over: ranked results list

**3.4 Server Config**

- Badge can request lower `update_rate` in `start` message to reduce bandwidth
- Server URL configurable (for local dev vs production)

---

## Testing

### Native Unit Tests (Phase 1 only — pure C++)

| Suite | Tests |
|-------|-------|
| test_physics | gravity, thrust (up + angled), rotation (rate + clamp), fuel depletion, empty fuel, position update |
| test_terrain | point count, zone exists, zone width, height interpolation, zone detection, y bounds |
| test_collision | above/at terrain, safe landing, too fast, bad angle, off zone, out of bounds |
| test_scoring | crash=0, perfect easy/hard, no fuel bonus, slow time bonus |
| test_game_state | init values, phase transitions (menu→playing→landed/crashed), no-op after end |

### Hardware Tests (Manual)

#### Display
- [ ] Game screen loads clean
- [ ] Terrain visible and continuous
- [ ] Lander rotates smoothly
- [ ] Camera follows lander
- [ ] HUD readable
- [ ] Landing zone visually distinct

#### Touch Input
- [ ] Top half → thrust
- [ ] Bottom-left → rotate left
- [ ] Bottom-right → rotate right
- [ ] Enter button → thrust
- [ ] Back button → exits to menu
- [ ] No touch → gravity only

#### Gameplay
- [ ] Falls under gravity from top-center
- [ ] Thrust: flame + acceleration
- [ ] Fuel depletes, thrust stops at 0
- [ ] Safe landing → score
- [ ] Too fast / bad angle / off zone / off screen → crash
- [ ] Difficulty select works
- [ ] Play Again + Menu return work

#### Audio & LEDs
- [ ] Thrust tone while thrusting
- [ ] Landing jingle, crash tone
- [ ] NeoPixels per state (orange/green/red)

#### Stability
- [ ] 10 consecutive games, no crash
- [ ] Heap stable across games
- [ ] Menu exit + re-enter repeatable
- [ ] OTA / QA mode / other menu items unaffected

#### Spectator (Phase 2)
- [ ] Connects to server over WiFi
- [ ] Session list loads
- [ ] Single-player spectate renders correctly
- [ ] Multiplayer spectate renders all players
- [ ] Game over results display
- [ ] Disconnect/reconnect handled gracefully

#### Multiplayer (Phase 3)
- [ ] Room list loads
- [ ] Create room works
- [ ] Join room works
- [ ] Waiting lobby shows players
- [ ] Countdown + game start
- [ ] Touch input controls server-side lander
- [ ] All players visible
- [ ] Game over rankings display
- [ ] Player disconnect handled

### Performance Targets
- [ ] Physics tick < 1ms
- [ ] Frame render < 30ms
- [ ] Canvas in PSRAM
- [ ] Game heap overhead < 10KB (excl. canvas)

---

## Task Tracking

All tasks tracked as GitHub Issues on [carlfugate/lunarlander-badge](https://github.com/carlfugate/lunarlander-badge/issues).

See issues for current status.
