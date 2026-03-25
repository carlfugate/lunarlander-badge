# Serial Test Harness — Backport Guide

This document describes the serial command protocol for automated badge testing.
Designed to be backported to any ESP32/LVGL badge project.

## Badge Side (Firmware)

Copy these files into your project:
- `include/QA/SerialCmd.h`
- `src/QA/SerialCmd.cpp`

Add to your `loop()`:
```cpp
serial_cmd_poll();
```

Add to your `setup()`:
```cpp
serial_cmd_init();
```

Guard with feature flag:
```cpp
// platformio.ini build_flags:
// -DFF_SERIAL_TEST    ; enable for test builds
```

## Protocol

Commands are newline-terminated ASCII strings sent over USB serial.
Responses are structured: `[TAG] key=value key=value`

### Commands
| Command | Response Tag | Description |
|---------|-------------|-------------|
| heap | [HEAP] | Free and minimum heap |
| state | [STATE] | Current state summary |
| version | [VERSION] | Firmware version |
| nav <screen> | [NAV] | Navigate to screen |
| tap <x> <y> | [TAP] | Inject touch event |
| back | [NAV] | Go to main menu |
| stress <n> | [STRESS] | Cycle screens n times |
| idle <ms> | [IDLE] | Wait with LVGL running |
| reboot | [REBOOT] | Software reset |
| help | [HELP] | List commands |

### Automatic Telemetry
Emitted on every screen transition:
```
[NAV] screen_change heap=236288
```

## Host Side (MCP Server)

See `tools/badge-mcp/` for the MCP server that bridges serial to AI tools.
Install: `pip install mcp pyserial`
Run: `python tools/badge-mcp/server.py --port /dev/cu.usbserial-10`

## Adapting for Your Badge

1. Copy SerialCmd.h/cpp into your project
2. In SerialCmd.cpp `cmd_nav()`, register your badge's screens
3. Add `serial_cmd_poll()` to your `loop()`
4. Add `serial_cmd_init()` to your `setup()`
5. Add `serial_cmd_log("NAV", ...)` to your screen transition function
6. Build with `-DFF_SERIAL_TEST`
