# Badge Serial Test Protocol (BSTP) v1.0

A standard for automated hardware testing of ESP32/LVGL conference badges via serial.
Designed for AI-driven testing via MCP, backportable to any badge project.

## Quick Start

```bash
# Build with test harness enabled
pio run  # FF_SERIAL_TEST defined in platformio.ini

# Connect serial monitor
pio device monitor -b 115200

# Type commands
help
sys.heap
nav.main
bling.set 1
bling.status
```

## Architecture

### Registration Pattern

Each module registers its own serial commands. No central dispatcher to maintain.

```cpp
// In your_module.cpp, at the end:
#ifdef FF_SERIAL_TEST
#include "QA/SerialCmd.h"
static void my_handler(const char *args) {
    if (strcmp(args, "status") == 0) {
        serial_cmd_log("MYMOD", "value=%d", some_value);
    } else if (strcmp(args, "list") == 0) {
        serial_cmd_log("MYMOD", "options=a,b,c");
    } else {
        serial_cmd_log("MYMOD", "error=unknown args=%s", args);
    }
}
void serial_register_mymodule() {
    serial_cmd_register("mymod", my_handler, "status, list");
}
#else
void serial_register_mymodule() {}
#endif
```

Then in `serial_cmd_init()` add:
```cpp
extern void serial_register_mymodule();
serial_register_mymodule();
```

### Response Format

All responses are structured:
```
[TAG] key=value key=value ...
```

Special tags:
- `[OK]` — success, no data
- `[ERROR]` — failure with `reason=...`
- `[NAV]` — automatic screen transition telemetry
- `[INIT]` — boot message
- `[TEST]` — test harness progress

## Command Reference

### sys — System
| Command | Response | Description |
|---------|----------|-------------|
| sys.heap | [SYS] free=N min=N | Current and minimum free heap |
| sys.version | [SYS] firmware=X codename=X | Firmware version |
| sys.uptime | [SYS] ms=N | Milliseconds since boot |
| sys.reboot | [SYS] rebooting | Software reset |

### nav — Navigation
| Command | Description |
|---------|-------------|
| nav.main | Main menu |
| nav.system | System submenu |
| nav.bling | LED array control |
| nav.wifi | WiFi status |
| nav.callsign | Crew identification |
| nav.achievements | Mission patches |
| nav.crew | Crew roster |
| nav.comms | BLE comms |
| nav.game | Lunar Lander |
| nav.schedule | Mission schedule |
| nav.battery | Battery status |
| nav.buzzer | Buzzer test |
| nav.sd | SD card browser |
| nav.info | System info |
| nav.ota | OTA update |
| nav.credits | Credits |
| nav.card | Badge card |
| nav.screensaver | Screensaver picker |
| nav.checkin | Check-in |

### bling — LED Animations
| Command | Description |
|---------|-------------|
| bling.set N | Activate mode N (0-11) |
| bling.off | Stop animation |
| bling.status | Current mode |
| bling.list | List all modes with names |

### ble — Bluetooth Presence
| Command | Description |
|---------|-------------|
| ble.status | Nearby/total badge count |
| ble.send N | Send preset message N (1-12) |
| ble.crew | List discovered badges |

### callsign — Identity
| Command | Description |
|---------|-------------|
| callsign.get | Current callsign |
| callsign.set NAME | Set callsign |

### audio — Sound
| Command | Description |
|---------|-------------|
| audio.mute | Mute all sound |
| audio.unmute | Unmute |
| audio.status | Mute state |

### screensaver — Display
| Command | Description |
|---------|-------------|
| screensaver.mode N | Set mode (0-3) |
| screensaver.trigger | Reset idle timer |
| screensaver.status | Current mode |
| screensaver.list | List modes |

### achievements — Progress
| Command | Description |
|---------|-------------|
| achievements.list | All achievements with status |
| achievements.unlock N | Force-unlock achievement N |
| achievements.status | Total unlocked + games played |

### test — Test Harness
| Command | Description |
|---------|-------------|
| test.stress N | Cycle screens N times, report heap delta |
| test.idle MS | Idle for MS milliseconds, report heap delta |

### help — Discovery
| Command | Description |
|---------|-------------|
| help | List all registered modules |
| help MODULE | List commands for a module |

## Feature Flag

```cpp
// include/FeatureFlags.h
#define FF_SERIAL_TEST  // Enable for test builds

// platformio.ini
build_flags = -DFF_SERIAL_TEST
```

Remove for production builds. When undefined, all serial test code compiles to nothing (inline no-ops).

## Adding a New Feature (Checklist)

When adding a new badge feature:

1. [ ] Add serial handler block at end of your `.cpp` file (guarded by `FF_SERIAL_TEST`)
2. [ ] Implement subcommands: at minimum `status` and `list` (if applicable)
3. [ ] Register in `serial_cmd_init()` via `extern void serial_register_xxx(); serial_register_xxx();`
4. [ ] Add to nav handler if the feature has a screen
5. [ ] Update this document's command reference
6. [ ] Add test scenario to MCP server if needed

## MCP Server

See `tools/badge-mcp/` for the AI-facing MCP server that bridges serial to tools.

## Backporting to Other Projects

1. Copy `include/QA/SerialCmd.h` and `src/QA/SerialCmd.cpp`
2. Add `serial_cmd_init()` to `setup()`, `serial_cmd_poll()` to `loop()`
3. Add `FF_SERIAL_TEST` to your feature flags
4. Register your modules' handlers
5. Copy `tools/badge-mcp/` for the MCP server
