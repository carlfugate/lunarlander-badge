#!/usr/bin/env python3
"""Badge Test MCP Server — bridges serial to AI tools for hardware testing.

Backportable to any ESP32/LVGL badge with the SerialCmd firmware module.

Usage:
    pip install mcp pyserial
    python server.py --port /dev/cu.usbserial-10 --baud 115200
"""
import asyncio
import argparse
import time
from collections import deque
from typing import Optional

import serial
from mcp.server import Server
from mcp.types import Tool, TextContent
from mcp.server.stdio import stdio_server

app = Server("badge-test")

# Serial connection
ser: Optional[serial.Serial] = None
log_buffer: deque = deque(maxlen=500)


def serial_send(cmd: str) -> str:
    """Send command and collect response lines for up to 2 seconds."""
    if not ser or not ser.is_open:
        return "[ERROR] Serial not connected"

    # Drain any pending input
    while ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line:
            log_buffer.append(line)

    ser.write(f"{cmd}\n".encode())
    ser.flush()

    lines = []
    deadline = time.time() + 2.0
    while time.time() < deadline:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if line:
                log_buffer.append(line)
                lines.append(line)
                # If we got a response tag matching our command, we're done
                if line.startswith('['):
                    deadline = time.time() + 0.2  # short grace period for extra lines
        else:
            time.sleep(0.05)

    return '\n'.join(lines) if lines else "[TIMEOUT] No response"


def serial_monitor(duration_ms: int) -> str:
    """Capture all serial output for duration."""
    if not ser or not ser.is_open:
        return "[ERROR] Serial not connected"

    lines = []
    deadline = time.time() + duration_ms / 1000.0
    while time.time() < deadline:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if line:
                log_buffer.append(line)
                lines.append(line)
        else:
            time.sleep(0.05)

    return '\n'.join(lines) if lines else "(no output)"


@app.list_tools()
async def list_tools():
    return [
        Tool(
            name="badge_send",
            description="Send a raw serial command to the badge and return the response. Commands: heap, state, version, reboot, nav <screen>, tap <x> <y>, stress <n>, idle <ms>, back, help",
            inputSchema={
                "type": "object",
                "properties": {
                    "command": {"type": "string", "description": "Serial command to send"}
                },
                "required": ["command"]
            }
        ),
        Tool(
            name="badge_navigate",
            description="Navigate to a screen and verify arrival. Screens: main, system, bling, callsign, achievements, crew, comms, game",
            inputSchema={
                "type": "object",
                "properties": {
                    "screen": {"type": "string", "description": "Screen name"}
                },
                "required": ["screen"]
            }
        ),
        Tool(
            name="badge_heap",
            description="Get current heap memory stats (free and minimum)",
            inputSchema={"type": "object", "properties": {}}
        ),
        Tool(
            name="badge_monitor",
            description="Capture all serial output for a duration (for idle/stability testing)",
            inputSchema={
                "type": "object",
                "properties": {
                    "duration_ms": {"type": "integer", "description": "Duration in milliseconds", "default": 5000}
                }
            }
        ),
        Tool(
            name="badge_stress_test",
            description="Run a navigation stress test — rapidly cycle through screens N times and report heap delta",
            inputSchema={
                "type": "object",
                "properties": {
                    "cycles": {"type": "integer", "description": "Number of cycles", "default": 50}
                }
            }
        ),
        Tool(
            name="badge_idle_test",
            description="Let badge idle for duration and monitor for crashes/heap leaks",
            inputSchema={
                "type": "object",
                "properties": {
                    "duration_ms": {"type": "integer", "description": "Idle duration in ms", "default": 60000}
                }
            }
        ),
        Tool(
            name="badge_log",
            description="Get the recent serial log buffer (last 500 lines)",
            inputSchema={"type": "object", "properties": {}}
        ),
        Tool(
            name="badge_run_test",
            description="Run a named test scenario: navigation_cycle, screensaver_test, game_test, full_suite",
            inputSchema={
                "type": "object",
                "properties": {
                    "test_name": {"type": "string", "description": "Test name"}
                },
                "required": ["test_name"]
            }
        ),
    ]


@app.call_tool()
async def call_tool(name: str, arguments: dict):
    if name == "badge_send":
        result = serial_send(arguments["command"])
        return [TextContent(type="text", text=result)]

    elif name == "badge_navigate":
        result = serial_send(f"nav {arguments['screen']}")
        return [TextContent(type="text", text=result)]

    elif name == "badge_heap":
        result = serial_send("heap")
        return [TextContent(type="text", text=result)]

    elif name == "badge_monitor":
        duration = arguments.get("duration_ms", 5000)
        result = serial_monitor(duration)
        return [TextContent(type="text", text=result)]

    elif name == "badge_stress_test":
        cycles = arguments.get("cycles", 50)
        result = serial_send(f"stress {cycles}")
        extra = serial_monitor(cycles * 200 + 5000)
        return [TextContent(type="text", text=f"{result}\n{extra}")]

    elif name == "badge_idle_test":
        duration = arguments.get("duration_ms", 60000)
        result = serial_send(f"idle {duration}")
        extra = serial_monitor(duration + 5000)
        return [TextContent(type="text", text=f"{result}\n{extra}")]

    elif name == "badge_log":
        return [TextContent(type="text", text='\n'.join(log_buffer) if log_buffer else "(empty)")]

    elif name == "badge_run_test":
        test_name = arguments["test_name"]
        results = []

        if test_name == "navigation_cycle":
            screens = ["system", "bling", "callsign", "achievements", "crew", "comms", "main"]
            initial = serial_send("heap")
            results.append(f"Initial: {initial}")
            for s in screens:
                r = serial_send(f"nav {s}")
                results.append(r)
            serial_send("nav main")
            final = serial_send("heap")
            results.append(f"Final: {final}")

        elif test_name == "screensaver_test":
            results.append(serial_send("heap"))
            results.append("Idling 65s for screensaver...")
            results.append(serial_monitor(66000))
            results.append(serial_send("tap 160 120"))
            results.append(serial_send("heap"))

        elif test_name == "game_test":
            results.append(serial_send("nav game"))
            results.append(serial_monitor(3000))
            results.append(serial_send("back"))
            results.append(serial_send("heap"))

        elif test_name == "full_suite":
            for sub in ["navigation_cycle", "game_test"]:
                r = await call_tool("badge_run_test", {"test_name": sub})
                results.append(f"--- {sub} ---")
                results.append(r[0].text)

        else:
            results.append(f"Unknown test: {test_name}")
            results.append("Available: navigation_cycle, screensaver_test, game_test, full_suite")

        return [TextContent(type="text", text='\n'.join(results))]

    return [TextContent(type="text", text=f"Unknown tool: {name}")]


async def main():
    global ser
    parser = argparse.ArgumentParser(description="Badge Test MCP Server")
    parser.add_argument("--port", default="/dev/cu.usbserial-10", help="Serial port")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    args = parser.parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
        print(f"Connected to {args.port} at {args.baud}", file=__import__('sys').stderr)
    except Exception as e:
        print(f"Warning: Could not open {args.port}: {e}", file=__import__('sys').stderr)
        print("Running without serial connection (tools will return errors)", file=__import__('sys').stderr)

    async with stdio_server() as (read_stream, write_stream):
        await app.run(read_stream, write_stream, app.create_initialization_options())


if __name__ == "__main__":
    asyncio.run(main())
