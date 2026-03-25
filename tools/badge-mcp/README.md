# Badge Test MCP Server

AI-driven hardware testing for ESP32/LVGL conference badges via serial.

## Setup

```bash
cd tools/badge-mcp
pip install -e .
```

## Usage with Kiro CLI

Add to `~/.kiro/settings.json`:
```json
{
  "mcpServers": {
    "badge-test": {
      "command": "python",
      "args": ["tools/badge-mcp/server.py", "--port", "/dev/cu.usbserial-10"]
    }
  }
}
```

## Badge Firmware Requirement

The badge must be built with `FF_SERIAL_TEST` defined. This enables the serial command handler.

## Available Tools

| Tool | Description |
|------|-------------|
| badge_send | Send raw serial command |
| badge_navigate | Navigate to a screen |
| badge_heap | Get heap stats |
| badge_monitor | Capture serial output for duration |
| badge_stress_test | Rapid screen cycling stress test |
| badge_idle_test | Long idle stability test |
| badge_log | Get recent serial log |
| badge_run_test | Run named test scenario |

## Backporting

This server works with any badge that implements the serial command protocol.
See `docs/SERIAL_TEST_HARNESS.md` for the protocol spec.
