# Serial Test Harness — Centralized Implementation Spec

## Status: PLAN (not yet implemented)

This spec replaces the distributed `#ifdef FF_SERIAL_TEST` pattern documented in
`docs/SERIAL_TEST_HARNESS.md`. That approach spread handler code across 8 module
files and caused bugs. This spec centralizes everything in `SerialCmd.cpp`.

---

## 1. What Went Wrong (Post-Mortem)

### 1.1 Distributed `#ifdef` blocks broke module files
Handler code was appended to the end of 8 `.cpp` files inside `#ifdef FF_SERIAL_TEST`
blocks. These handlers accessed file-scoped `static` variables and functions directly
(e.g., `bling_mode`, `start_bling_animation()` in Bling.cpp). This worked only because
the handler lived in the same translation unit — but it created tight coupling between
test code and module internals.

### 1.2 Static access caused the difficulty button bug
The game serial handler called `static start_game()` and read `static GameState gs`
directly. This bypassed the normal UI flow (`show_mode_select()` →
`show_difficulty_select()` → `diff_btn_cb()` → `start_game()`), leaving static state
like `pending_mode`, `game_screen`, and `mode_label` in inconsistent states.

### 1.3 Files with `#ifdef` blocks that must be cleaned up

| File | What the block accesses | Static? |
|------|------------------------|---------|
| `src/QA/Menu.cpp` | `create_ota_window()`, `create_screensaver_picker()` | `create_screensaver_picker` is static, `create_ota_window` is non-static but not in any header |
| `src/QA/Bling.cpp` | `start_bling_animation()`, `bling_mode` | Both static |
| `src/QA/BlePresence.cpp` | `ble_presence_get_crew()` (public), `ble_presence_nearby_count()` (public) | OK — all public |
| `src/QA/Callsign.cpp` | `callsign_get()`, `callsign_set()` | OK — all public |
| `src/Game/LunarAudio.cpp` | `audio_set_mute()`, `audio_is_muted()` | OK — all public |
| `src/QA/Screensaver.cpp` | `screensaver_set_mode()`, `screensaver_get_mode()` | OK — all public |
| `src/QA/Achievements.cpp` | `achievement_unlocked()`, `achievement_unlock()`, `achievements_total()`, `ACH_COUNT` | OK — all public |

Only Menu.cpp and Bling.cpp access non-public symbols. Those are the dangerous ones.

### 1.4 No incremental hardware testing
All 8 module handlers were added in one commit. Bugs accumulated silently. The game
was never tested between adding serial handlers.

### 1.5 `test.idle` initially didn't call `serial_cmd_poll()`
The idle loop ran `lv_timer_handler()` + `delay()` but forgot `serial_cmd_poll()`,
making the badge unresponsive during idle tests. This was fixed later but illustrates
the risk of distributed code — easy to miss integration details.

---

## 2. Architecture

### 2.1 Single-file principle
ALL command handlers live in `src/QA/SerialCmd.cpp`. No other file is modified for
serial test support. The entire file is wrapped in `#ifdef FF_SERIAL_TEST` /
`#endif`.

### 2.2 Public API only
Serial handlers call ONLY functions declared in public headers. No `extern` of static
functions. No accessing static variables from other translation units.

### 2.3 Feature flag scope
`FF_SERIAL_TEST` guards exactly two things:
- The body of `src/QA/SerialCmd.cpp` (the implementation)
- The declarations in `include/QA/SerialCmd.h` (already has inline no-op fallbacks)

No other file contains `#ifdef FF_SERIAL_TEST`. Period.

### 2.4 Header contract
If a module needs a new capability exposed for serial testing, the change goes in:
1. The module's header (declare the function)
2. The module's `.cpp` (implement the function)
3. `SerialCmd.cpp` (call the function)

The module file is modified to add a public function, NOT to add a serial handler.

### 2.5 Current state of SerialCmd.cpp
The core framework already exists and works:
- Command buffer, parser, dispatcher
- `serial_cmd_register()` / `serial_cmd_poll()` / `serial_cmd_log()`
- Built-in: `sys.heap`, `sys.version`, `sys.uptime`, `sys.reboot`
- Built-in: `test.stress`, `test.idle`
- Built-in: `help`
- Wired into `main.cpp`: `serial_cmd_init()` in `setup()`, `serial_cmd_poll()` in `loop()`

What needs to change: remove the `extern void serial_register_*()` calls and the
distributed handlers, replace with centralized handlers in SerialCmd.cpp.

---

## 3. Public API Audit

### 3.1 APIs that are already public (no changes needed)

| Module | Header | Functions available |
|--------|--------|-------------------|
| Callsign | `QA/Callsign.h` | `callsign_get()`, `callsign_set()`, `create_callsign_window()` |
| Audio | `Game/LunarAudio.h` | `audio_set_mute()`, `audio_is_muted()` |
| Screensaver | `QA/Screensaver.h` | `screensaver_set_mode()`, `screensaver_get_mode()`, `screensaver_stop()`, `screensaver_reset_timer()` |
| Achievements | `QA/Achievements.h` | `achievement_unlocked()`, `achievement_unlock()`, `achievements_total()`, `achievements_games_played()`, `ACH_COUNT` |
| BLE | `QA/BlePresence.h` | `ble_presence_nearby_count()`, `ble_presence_total_count()`, `ble_presence_get_crew()`, `ble_presence_get_crew_count()`, `ble_presence_send_message()` |
| Navigation | `QA/Menu.h` | `create_main_menu()`, `create_system_submenu()`, `create_battery_window()`, `create_buzzer_window()`, `create_sd_card_window()`, `create_system_info_window()`, `create_credits_window()`, `create_badge_card_window()`, `create_checkin_window()` |
| Navigation | `QA/Bling.hpp` | `create_bling_window()`, `bling_stop_animation()` |
| Navigation | `QA/Callsign.h` | `create_callsign_window()` |
| Navigation | `QA/Achievements.h` | `create_achievements_window()` |
| Navigation | `QA/BlePresence.h` | `create_crew_log_window()`, `create_comms_window()` |
| Game | `Game/LunarState.h` | `lunar_lander_start()`, `lunar_lander_stop()` |
| Schedule | `QA/schedule.h` | `displaySchedule()` |

### 3.2 APIs that need to be added

| Module | Header to modify | New function | Purpose | Implementation |
|--------|-----------------|-------------|---------|----------------|
| Bling | `QA/Bling.hpp` | `void bling_set_mode(int mode)` | Start animation by mode number | Wrapper around static `start_bling_animation()` |
| Bling | `QA/Bling.hpp` | `int bling_get_mode()` | Read current bling mode | Return static `bling_mode` |
| Menu | `QA/Menu.h` | `void create_ota_window()` | Already non-static in Menu.cpp, just needs header declaration | None — already implemented, just undeclared |
| Game | `Game/LunarState.h` | `const GameState* game_get_state()` | Read-only access to game state for diagnostics | Return `&gs` (the static GameState) |

### 3.3 APIs that should NOT be added

| What | Why not |
|------|---------|
| `start_game(difficulty)` as public | Bypasses UI flow, leaves `game_screen`/`pending_mode` inconsistent. Use `lunar_lander_start()` which goes through the proper menu flow. |
| `show_difficulty_select()` as public | Same problem — partial UI state. |
| Direct thrust injection | `game_tick_cb` reads `input_read()` every frame. Injecting thrust would require either a global flag or modifying the input system. Too invasive for v1. |
| `create_screensaver_picker()` as public | Static in Menu.cpp, tightly coupled to menu UI. Navigate via `nav.screensaver` isn't critical — drop it or navigate to main menu instead. |

### 3.4 Game serial strategy

The game is the riskiest module. The safe approach:

- `game.state` — read-only via `game_get_state()`. Reports phase, score, fuel, difficulty, elapsed time. Zero side effects.
- `game.start` — calls `lunar_lander_start()` which is already public. This shows the mode select screen. The user (or a future touch-injection system) picks difficulty. We do NOT try to auto-start a game at a specific difficulty.
- `game.stop` — calls `lunar_lander_stop()` which is already public. Cleans up timers, returns to main menu.
- `game.thrust` — NOT IMPLEMENTED in v1. Would require modifying `LunarInput.cpp` to support a serial override flag. Deferred.

---

## 4. Implementation Steps

Each step is a single commit. Flash to hardware and verify after each one.

### Step 1: Remove distributed handlers

**Changes:**
- Delete the `#ifdef FF_SERIAL_TEST` ... `#endif` blocks from all 7 module files:
  - `src/QA/Menu.cpp`
  - `src/QA/Bling.cpp`
  - `src/QA/BlePresence.cpp`
  - `src/QA/Callsign.cpp`
  - `src/Game/LunarAudio.cpp`
  - `src/QA/Screensaver.cpp`
  - `src/QA/Achievements.cpp`
- In `src/QA/SerialCmd.cpp`, remove the `extern void serial_register_*()` declarations and calls from `serial_cmd_init()`

**Verify on hardware:**
- `help` → shows only sys, test, help
- `sys.heap` → works
- Game still launches and plays normally
- All menu screens still work

### Step 2: Add missing public APIs to module headers

**Changes:**
- `include/QA/Bling.hpp`: add `void bling_set_mode(int mode);` and `int bling_get_mode();`
- `src/QA/Bling.cpp`: implement `bling_set_mode()` as wrapper around `start_bling_animation()`, implement `bling_get_mode()` returning `bling_mode`
- `include/QA/Menu.h`: add declaration `void create_ota_window();`
- `include/Game/LunarState.h`: add `const GameState* game_get_state();`
- `src/Game/LunarState.cpp`: implement `const GameState* game_get_state() { return &gs; }` (inside the `#ifndef NATIVE_TEST` block)

**Verify on hardware:**
- Build succeeds
- Game still launches and plays normally (no behavior change — new functions aren't called yet)

### Step 3: Add navigation handlers to SerialCmd.cpp

**Changes:**
- Add `#include` for `QA/Menu.h`, `QA/Bling.hpp`, `QA/Callsign.h`, `QA/Achievements.h`, `QA/BlePresence.h`, `QA/schedule.h`, `Game/LunarState.h`
- Add `nav_handler()` in SerialCmd.cpp calling only public `create_*()` functions
- Supported: `nav.main`, `nav.system`, `nav.battery`, `nav.buzzer`, `nav.sd`, `nav.info`, `nav.ota`, `nav.credits`, `nav.card`, `nav.checkin`, `nav.bling`, `nav.wifi`, `nav.callsign`, `nav.achievements`, `nav.crew`, `nav.comms`, `nav.game`, `nav.schedule`
- Register in `serial_cmd_init()`
- Drop `nav.screensaver` (was calling static `create_screensaver_picker()`)

**Verify on hardware:**
- `help` → shows nav module
- `nav.main` → main menu appears
- `nav.system` → system submenu appears
- `nav.bling` → bling window appears
- `nav.game` → game mode select appears
- Navigate back to main menu via touch, verify game still works

### Step 4: Add module status handlers to SerialCmd.cpp

**Changes:**
- Add `bling_handler()`: `bling.set <n>`, `bling.off`, `bling.status`, `bling.list`
  - Uses new public `bling_set_mode()` and `bling_get_mode()`
- Add `audio_handler()`: `audio.mute`, `audio.unmute`, `audio.status`
  - Uses existing public `audio_set_mute()`, `audio_is_muted()`
- Add `callsign_handler()`: `callsign.get`, `callsign.set <name>`
  - Uses existing public `callsign_get()`, `callsign_set()`
- Add `screensaver_handler()`: `screensaver.mode <n>`, `screensaver.status`, `screensaver.list`
  - Uses existing public `screensaver_set_mode()`, `screensaver_get_mode()`
- Add `ach_handler()`: `achievements.list`, `achievements.unlock <id>`, `achievements.status`
  - Uses existing public `achievement_unlocked()`, `achievement_unlock()`, `achievements_total()`, `achievements_games_played()`
- Add `ble_handler()`: `ble.status`, `ble.send <n>`, `ble.crew`
  - Uses existing public `ble_presence_nearby_count()`, `ble_presence_total_count()`, `ble_presence_get_crew()`, `ble_presence_get_crew_count()`, `ble_presence_send_message()`
  - Note: BLE is currently disabled (see commit d179397). Handler should still compile but will report zeros.
- Register all in `serial_cmd_init()`

**Verify on hardware:**
- `bling.set 1` → rainbow animation starts
- `bling.status` → reports `mode=1`
- `bling.off` → LEDs off
- `audio.mute` / `audio.unmute` / `audio.status` → work
- `callsign.get` → returns current callsign
- `screensaver.status` → reports current mode
- `achievements.status` → reports total/games
- Game still launches and plays normally

### Step 5: Add game state handler to SerialCmd.cpp

**Changes:**
- Add `game_handler()`:
  - `game.state` → calls `game_get_state()`, reports phase, mode, difficulty, score, fuel, elapsed_ms. If phase is PHASE_MENU, report "not_playing".
  - `game.start` → calls `lunar_lander_start()` (shows mode select, does NOT auto-pick difficulty)
  - `game.stop` → calls `lunar_lander_stop()` (cleans up, returns to main menu)
- Register in `serial_cmd_init()`

**Verify on hardware:**
- `game.state` → reports `phase=menu` when not playing
- `game.start` → mode select screen appears
- Pick difficulty via touch, play game
- `game.state` → reports `phase=playing score=0 fuel=...`
- `game.stop` → returns to main menu
- Start game again via touch (NOT serial) → verify difficulty buttons work correctly
- Play full game to completion → verify game over screen, scores, achievements all work

### Step 6: Update documentation

**Changes:**
- Update `docs/SERIAL_TEST_HARNESS.md` to reflect centralized architecture
- Remove references to per-module registration pattern
- Update "Adding a New Feature" checklist
- Note dropped commands (`nav.screensaver`, `game.thrust`)

**Verify:** documentation review only, no hardware test needed.

---

## 5. Command Reference (Post-Migration)

### Retained (same behavior)
- `sys.heap`, `sys.version`, `sys.uptime`, `sys.reboot`
- `test.stress <n>`, `test.idle <ms>`
- `help`, `help <module>`
- `nav.*` (all screens except `nav.screensaver` — dropped)
- `bling.set <n>`, `bling.off`, `bling.status`, `bling.list`
- `ble.status`, `ble.send <n>`, `ble.crew`
- `callsign.get`, `callsign.set <name>`
- `audio.mute`, `audio.unmute`, `audio.status`
- `screensaver.mode <n>`, `screensaver.status`, `screensaver.list`
- `achievements.list`, `achievements.unlock <id>`, `achievements.status`

### Changed
- `screensaver.trigger` → renamed to `screensaver.reset` (calls `screensaver_reset_timer()`)

### Added
- `game.state` — read-only game state
- `game.start` — enter game (mode select screen)
- `game.stop` — exit game, return to main menu

### Dropped
- `nav.screensaver` — called static `create_screensaver_picker()`, no public API
- `game.thrust` — deferred, requires input system changes

---

## 6. Key Design Rules

1. ALL handlers in `SerialCmd.cpp` — one file, one `#ifdef` guard
2. Only call functions declared in public headers
3. Never access static variables from other translation units
4. Never modify module `.cpp` files to add serial handlers
5. If a module needs a new public API, add it to the header + implement in the module — the serial handler stays in `SerialCmd.cpp`
6. `test.idle` MUST call `serial_cmd_poll()` in its loop (already does)
7. `FF_SERIAL_TEST` guards `SerialCmd.cpp` and `SerialCmd.h` — nothing else
8. Hardware test after EVERY step — especially the game
9. Game handlers are read-only or use existing public start/stop — never bypass the UI flow

---

## 7. Risk Mitigations

| Risk | Mitigation |
|------|-----------|
| Preprocessor guard imbalance | Single `#ifdef`/`#endif` pair in one file |
| Game state corruption | `game.state` is read-only; `game.start`/`game.stop` use existing public API that goes through proper UI flow |
| Difficulty button bug recurrence | Never call `start_game()` directly — it's static and stays static |
| Feature flag leaks to production | `FF_SERIAL_TEST` only in `platformio.ini` build_flags; `SerialCmd.h` compiles to inline no-ops when undefined |
| Bling handler breaks animations | New `bling_set_mode()` is a thin wrapper — same code path as UI buttons |
| BLE handler crashes (BLE disabled) | Public BLE functions return 0/nullptr when BLE is off — handler reports zeros gracefully |
| Accumulated bugs from batch changes | Each step is one commit, tested on hardware before proceeding |
| Module API changes break native tests | New public functions are inside `#ifndef NATIVE_TEST` guards where needed (game state getter) |

---

## 8. File Change Summary

### Files modified (to add public APIs — Step 2)
- `include/QA/Bling.hpp` — add `bling_set_mode()`, `bling_get_mode()`
- `src/QA/Bling.cpp` — implement those two functions
- `include/QA/Menu.h` — add `create_ota_window()` declaration
- `include/Game/LunarState.h` — add `game_get_state()`
- `src/Game/LunarState.cpp` — implement `game_get_state()`

### Files modified (to remove distributed handlers — Step 1)
- `src/QA/Menu.cpp` — delete `#ifdef FF_SERIAL_TEST` block (~30 lines)
- `src/QA/Bling.cpp` — delete `#ifdef FF_SERIAL_TEST` block (~22 lines)
- `src/QA/BlePresence.cpp` — delete `#ifdef FF_SERIAL_TEST` block (~23 lines)
- `src/QA/Callsign.cpp` — delete `#ifdef FF_SERIAL_TEST` block (~16 lines)
- `src/Game/LunarAudio.cpp` — delete `#ifdef FF_SERIAL_TEST` block (~12 lines)
- `src/QA/Screensaver.cpp` — delete `#ifdef FF_SERIAL_TEST` block (~23 lines)
- `src/QA/Achievements.cpp` — delete `#ifdef FF_SERIAL_TEST` block (~21 lines)

### Files rewritten (centralized handlers — Steps 3-5)
- `src/QA/SerialCmd.cpp` — remove extern registration calls, add all handlers inline

### Files NOT modified
- `include/QA/SerialCmd.h` — no changes needed
- `src/main.cpp` — no changes needed (already wired up)
- `include/FeatureFlags.h` — no changes needed
- `platformio.ini` — no changes needed

---

## 9. Native Testing Strategy

### 9.1 Problem

Flashing the ESP32-S3 takes 30-60s at 115200 baud (~1.9MB firmware), requires
bootloader mode (physical button press), and sometimes fails (chip stops responding).
We flashed ~15 times debugging the serial harness last round. Every bug caught before
flashing saves 1-2 minutes and eliminates a failure risk.

### 9.2 What's already testable natively

The `[env:native]` PlatformIO environment compiles with `-DNATIVE_TEST` and builds
only `src/Game/*` (via `build_src_filter = +<Game/*>`). Seven test suites exist:

| Test | What it covers |
|------|---------------|
| `test_collision` | Terrain collision detection |
| `test_game_state` | GameState init, phase transitions |
| `test_scoring` | Score calculation |
| `test_physics` | Lander physics (gravity, thrust, rotation) |
| `test_terrain` | Terrain generation |
| `test_scoreboard` | Scoreboard logic |
| `test_audio_stubs` | Audio mute/unmute (NATIVE_TEST stubs) |

These all pass without any hardware. The native env does NOT currently build any
`src/QA/*` files.

### 9.3 What can be tested natively for SerialCmd

The command parser and dispatcher in `SerialCmd.cpp` are logically hardware-independent:
- `serial_cmd_register()` — adds prefix + handler to a table
- `process_command()` — matches prefix, splits args, dispatches
- Command routing — correct handler called with correct args

What CANNOT be tested natively (hardware-dependent):
- `serial_cmd_poll()` — reads from `Serial` (Arduino)
- `serial_cmd_log()` — writes to `Serial`
- `sys_handler()` — calls `ESP.getFreeHeap()`, `ESP.restart()`, `millis()`
- `test_handler()` — calls `lv_timer_handler()`, `ESP.getFreeHeap()`, `delay()`
- All module handlers — call LVGL, ESP, NeoPixel APIs

### 9.4 Native test design for SerialCmd

Split `SerialCmd.cpp` into testable and non-testable parts:

```cpp
// --- Hardware-independent (testable natively) ---
// Command table, registration, prefix matching, arg splitting
void serial_cmd_register(const char *prefix, serial_cmd_handler_t handler, const char *help_text);
static void process_command(char *cmd);  // needs to be non-static or wrapped

// --- Hardware-dependent (ESP32 only) ---
// serial_cmd_poll(), serial_cmd_log(), all handlers
```

A native test would:
1. Call `serial_cmd_register()` with mock handlers
2. Feed command strings into `process_command()`
3. Assert the correct mock handler was called with correct args
4. Assert unknown commands produce error dispatch

To make this work:
- `process_command()` must be exposed (non-static or via a testable wrapper)
- `serial_cmd_log()` needs a `NATIVE_TEST` stub (printf to stdout)
- The native env needs to build SerialCmd.cpp: add `+<QA/SerialCmd.cpp>` to
  `build_src_filter` and define `FF_SERIAL_TEST` in native build flags
- Handler implementations stay guarded by `#ifndef NATIVE_TEST`

Estimated effort: small. The parser is ~20 lines. The test would be ~50 lines.
Catches: registration bugs, prefix matching bugs, arg splitting bugs, off-by-one
in command buffer.

### 9.5 What native tests would NOT catch

- Handlers calling wrong public API (compiles fine, wrong behavior)
- LVGL screen creation side effects
- Memory leaks from LVGL widgets
- Serial I/O timing issues
- BLE/WiFi interaction bugs

These require hardware testing — but they're in the handler implementations, not the
framework. If the framework is solid (tested natively), handler bugs are isolated and
easier to diagnose on hardware.

---

## 10. Flash Minimization Plan

### 10.1 Current problem

The implementation spec (Section 4) has 6 steps, each requiring a flash. At 30-60s
per flash plus potential failures, that's 3-6 minutes of flashing alone, plus risk of
bricking a flash cycle.

### 10.2 Revised flash strategy: 2 flashes instead of 6

**Flash 1: Framework + sys commands (Steps 1-2 combined)**
- Remove distributed handlers from module files
- Remove `extern serial_register_*()` calls from SerialCmd.cpp
- Add new public APIs to module headers (bling_set_mode, etc.)
- Verify on hardware:
  - `help` → shows sys, test, help only
  - `sys.heap` → responds correctly
  - Game launches, plays, difficulty buttons work
  - All menu screens still work

**Flash 2: All module handlers (Steps 3-5 combined)**
- Add nav, bling, audio, callsign, screensaver, achievements, ble, game handlers
- All in one commit — they're all in SerialCmd.cpp calling only public APIs
- Verify on hardware:
  - All serial commands respond
  - Game still works after serial commands
  - No memory leaks (sys.heap before/after)

### 10.3 Why combining Steps 3-5 is safe

- All handler code lives in one file (`SerialCmd.cpp`)
- All handlers call only public APIs (verified by Section 3 audit)
- No handler modifies module internals or static state
- If one handler has a bug, it doesn't affect others (isolated dispatch)
- The framework (registration, parsing, dispatch) is already proven from Flash 1
- Native tests (Section 9) validate the framework before Flash 2

### 10.4 Pre-flash gate: native validation

Before each flash, run the full native validation (see Section 13 for checklist).
This catches compile errors, linker errors (calling static functions), and parser
bugs without touching hardware.

### 10.5 When to add a flash

Add a third flash ONLY if:
- Flash 2 reveals a bug that requires framework changes (not just handler fixes)
- A handler causes a crash or hang (not just wrong output)
- Game behavior is affected (difficulty buttons, scoring, etc.)

Handler output bugs (wrong format, missing field) can be fixed and re-flashed as
part of the same Flash 2 cycle — don't count fix-and-reflash as a separate planned
flash.

---

## 11. Post-Flash Verification Script

### 11.1 Purpose

After each flash, run an automated 30-second check that confirms the badge is
functional. This replaces ad-hoc manual testing and ensures nothing regressed.

### 11.2 Script: `scripts/verify_flash.sh`

```bash
#!/bin/bash
# Post-flash verification — run immediately after successful flash
# Usage: ./scripts/verify_flash.sh [port]
# Default port: /dev/cu.usbmodem* (macOS) or /dev/ttyACM0 (Linux)

PORT="${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}"
if [ -z "$PORT" ]; then
    PORT="${1:-/dev/ttyACM0}"
fi
BAUD=115200
TIMEOUT=5
PASS=0
FAIL=0

send_cmd() {
    local cmd="$1"
    local expect="$2"
    echo "$cmd" > "$PORT"
    sleep 0.5
    # Read response with timeout
    local resp
    resp=$(timeout "$TIMEOUT" head -1 < "$PORT" 2>/dev/null)
    if echo "$resp" | grep -q "$expect"; then
        echo "  PASS: $cmd -> $resp"
        ((PASS++))
    else
        echo "  FAIL: $cmd -> expected '$expect', got '$resp'"
        ((FAIL++))
    fi
}

echo "=== Post-Flash Verification ==="
echo "Port: $PORT"
echo ""

# Configure serial port
stty -f "$PORT" "$BAUD" cs8 -cstopb -parenb 2>/dev/null || \
    stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb 2>/dev/null

# Wait for boot
echo "Waiting for boot..."
sleep 3

# Core checks
echo "--- Core ---"
send_cmd "sys.heap" "free="
send_cmd "sys.version" "firmware="
send_cmd "sys.uptime" "ms="
send_cmd "help" "modules="

# Module checks (only after Flash 2)
if [ "${2}" = "--full" ]; then
    echo "--- Modules ---"
    send_cmd "bling.status" "mode="
    send_cmd "audio.status" "muted="
    send_cmd "callsign.get" "callsign="
    send_cmd "screensaver.status" "mode="
    send_cmd "achievements.status" "total="
    send_cmd "ble.status" "nearby="
    send_cmd "game.state" "phase="
fi

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
```

### 11.3 Usage

```bash
# After Flash 1 (framework only):
./scripts/verify_flash.sh

# After Flash 2 (all handlers):
./scripts/verify_flash.sh /dev/cu.usbmodem14101 --full
```

### 11.4 What it catches

- Badge didn't boot (no serial output)
- Serial command framework broken (no response to sys.heap)
- Handler registration failed (help shows wrong module count)
- Individual handler crash (timeout on specific command)
- Firmware version mismatch (sys.version check)

### 11.5 What it doesn't catch

- Visual/UI bugs (screen rendering, animations)
- Touch input issues
- Game physics or scoring bugs
- Memory leaks (would need repeated commands over time)
- BLE/WiFi functionality

For those, use `test.stress` and `test.idle` commands, plus manual visual inspection.

---

## 12. Simulator Testing

### 12.1 Current simulator state

The SDL2 simulator (`sim/sim_main.cpp`) runs the game engine natively with keyboard
input. It compiles with `-DNATIVE_TEST -DSIMULATOR` and links only `src/Game/*` files.
It has its own event loop using `SDL_PollEvent()` for keyboard input.

### 12.2 Can we add serial command testing to the sim?

Yes, with caveats. The sim's event loop is SDL-based. Adding stdin command processing
would require non-blocking stdin reads inside the SDL loop:

```cpp
// In sim main loop, after SDL_PollEvent:
#include <fcntl.h>
// Set stdin non-blocking at startup:
// fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
// Then in loop:
char line[128];
if (fgets(line, sizeof(line), stdin)) {
    process_command(line);  // reuse SerialCmd parser
}
```

This would let you type commands like `game.state` in the terminal while the sim
runs, and see output on stdout.

### 12.3 What sim testing would cover

- Game state queries (`game.state`) — verify state reporting without hardware
- Game start/stop lifecycle — verify `lunar_lander_start()`/`lunar_lander_stop()` equivalents
- Command parser correctness — same parser code, different I/O backend

### 12.4 What sim testing would NOT cover

- Navigation commands (no LVGL, no menu system)
- Bling/LED commands (no NeoPixel)
- BLE/WiFi commands
- Audio commands (stubbed out)
- Screensaver, achievements, callsign (QA modules not compiled)

### 12.5 Recommendation

Low priority for v1. The sim only covers game commands, which are the simplest
handlers (3 commands). Native unit tests (Section 9) give better coverage of the
parser framework. The sim is most useful for game physics debugging, not serial
harness testing.

If added later, keep it minimal: non-blocking stdin in the SDL loop, reuse
`process_command()`, print to stdout instead of Serial.

---

## 13. Pre-Flash Checklist

Run this checklist before every flash. Automate as `scripts/pre_flash_check.sh`.

### 13.1 Automated checks

```bash
#!/bin/bash
# Pre-flash validation — run before every flash
# Usage: ./scripts/pre_flash_check.sh

set -e
FAIL=0

echo "=== Pre-Flash Checklist ==="

# 1. ESP32 build succeeds
echo -n "1. ESP32 build... "
if pio run -e esp32-s3 --silent 2>&1 | tail -1 | grep -q "SUCCESS"; then
    echo "PASS"
else
    echo "FAIL"
    ((FAIL++))
fi

# 2. Native build succeeds
echo -n "2. Native build... "
if pio run -e native --silent 2>&1 | tail -1 | grep -q "SUCCESS"; then
    echo "PASS"
else
    echo "FAIL"
    ((FAIL++))
fi

# 3. Native tests pass
echo -n "3. Native tests... "
if pio test -e native --silent 2>&1 | tail -1 | grep -q "passed"; then
    echo "PASS"
else
    echo "FAIL"
    ((FAIL++))
fi

# 4. No new warnings (check for warning count)
echo -n "4. Build warnings... "
WARNS=$(pio run -e esp32-s3 2>&1 | grep -c "warning:" || true)
if [ "$WARNS" -eq 0 ]; then
    echo "PASS (0 warnings)"
else
    echo "WARN ($WARNS warnings — review before flashing)"
fi

# 5. FF_SERIAL_TEST only in allowed files
echo -n "5. FF_SERIAL_TEST scope... "
BAD_FILES=$(grep -rn 'FF_SERIAL_TEST' src/ include/ --include='*.cpp' --include='*.h' --include='*.hpp' \
    | grep -v 'SerialCmd\.\|FeatureFlags\.h' | wc -l | tr -d ' ')
if [ "$BAD_FILES" -eq 0 ]; then
    echo "PASS"
else
    echo "FAIL — FF_SERIAL_TEST found in unexpected files:"
    grep -rn 'FF_SERIAL_TEST' src/ include/ --include='*.cpp' --include='*.h' --include='*.hpp' \
        | grep -v 'SerialCmd\.\|FeatureFlags\.h'
    ((FAIL++))
fi

# 6. SerialCmd.cpp only calls public APIs (no extern of statics)
echo -n "6. No extern statics... "
EXTERNS=$(grep -c 'extern.*static\|extern void serial_register' src/QA/SerialCmd.cpp 2>/dev/null || true)
if [ "$EXTERNS" -eq 0 ]; then
    echo "PASS"
else
    echo "FAIL — extern declarations found in SerialCmd.cpp"
    grep 'extern.*static\|extern void serial_register' src/QA/SerialCmd.cpp
    ((FAIL++))
fi

# 7. Game source files unchanged (optional — skip if intentionally modified)
echo -n "7. Game files unchanged... "
GAME_CHANGES=$(git diff --name-only HEAD -- src/Game/ include/Game/ 2>/dev/null | wc -l | tr -d ' ')
if [ "$GAME_CHANGES" -eq 0 ]; then
    echo "PASS"
else
    echo "INFO — $GAME_CHANGES game file(s) modified (verify intentional):"
    git diff --name-only HEAD -- src/Game/ include/Game/ 2>/dev/null
fi

echo ""
if [ "$FAIL" -eq 0 ]; then
    echo "=== ALL CHECKS PASSED — safe to flash ==="
    exit 0
else
    echo "=== $FAIL CHECK(S) FAILED — fix before flashing ==="
    exit 1
fi
```

### 13.2 Manual checks (cannot automate)

- [ ] Badge is in bootloader mode (hold BOOT, press RESET)
- [ ] USB cable is data-capable (not charge-only)
- [ ] Serial monitor is closed (won't conflict with upload)
- [ ] Battery is charged (low battery can cause flash failures)

### 13.3 When to skip checks

Never skip checks 1-3 (build + tests). Checks 5-6 can be skipped if you haven't
touched SerialCmd.cpp or FeatureFlags.h. Check 7 is informational only.

---

## 14. CI Integration

### 14.1 Current CI state

The GitHub Actions workflow (`.github/workflows/platformio-build.yml`) runs only
`platformio run` (ESP32 build). It does NOT:
- Build the native environment
- Run native tests
- Build with `FF_SERIAL_TEST` enabled
- Check feature flag scope

### 14.2 Recommended CI additions

Update `.github/workflows/platformio-build.yml` to add:

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v4
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.x'
      - name: Install PlatformIO
        run: pip install platformio

      # Existing: ESP32 build
      - name: Build ESP32 firmware
        run: platformio run -e esp32-s3

      # NEW: Native build + tests
      - name: Build native
        run: platformio run -e native
      - name: Run native tests
        run: platformio test -e native

      # NEW: Feature flag scope check
      - name: Check FF_SERIAL_TEST scope
        run: |
          BAD=$(grep -rn 'FF_SERIAL_TEST' src/ include/ --include='*.cpp' --include='*.h' --include='*.hpp' \
            | grep -v 'SerialCmd\.\|FeatureFlags\.h' | wc -l)
          if [ "$BAD" -ne 0 ]; then
            echo "ERROR: FF_SERIAL_TEST found outside allowed files:"
            grep -rn 'FF_SERIAL_TEST' src/ include/ --include='*.cpp' --include='*.h' --include='*.hpp' \
              | grep -v 'SerialCmd\.\|FeatureFlags\.h'
            exit 1
          fi
```

### 14.3 What CI catches

| Check | Bug type caught |
|-------|----------------|
| ESP32 build | Syntax errors, missing includes, type mismatches |
| Native build | Same, plus linker errors from calling static functions |
| Native tests | Parser regressions, physics regressions, scoring regressions |
| FF_SERIAL_TEST scope | Distributed `#ifdef` blocks creeping back in |

### 14.4 What CI doesn't catch

- Runtime behavior on ESP32 (heap, timing, LVGL rendering)
- Hardware-specific bugs (SPI, I2C, BLE, WiFi)
- Visual regressions
- Serial command output format

These require hardware testing, which is why the post-flash verification script
(Section 11) exists.

### 14.5 Implementation priority

Adding native build + test to CI is the single highest-value change. It's 3 lines
of YAML and catches the entire class of "compiles on ESP32 but calls static
functions" bugs that caused the difficulty button issue. The feature flag scope
check is another 5 lines and prevents regression to the distributed handler pattern.
