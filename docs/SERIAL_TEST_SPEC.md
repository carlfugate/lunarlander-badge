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
