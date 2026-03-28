# BSidesKC Badge BLE Scanner

Scan for BSidesKC badge BLE advertisements from macOS.

## Setup
```bash
pip3 install bleak pyserial --break-system-packages
# For simulate mode only:
pip3 install pyobjc-framework-CoreBluetooth --break-system-packages
```

## Usage
```bash
# Quick scan (10 seconds)
open tools/ble-scanner/BLEScan.app

# Scan for 30 seconds
open tools/ble-scanner/BLEScan.app --args scan 30

# Continuous monitoring (Ctrl+C to stop)
open tools/ble-scanner/BLEScan.app --args monitor

# BLE integration test (scan + serial verification)
open tools/ble-scanner/BLEScan.app --args test /dev/cu.usbserial-110

# Simulate badge advertisement (limited — see below)
open tools/ble-scanner/BLEScan.app --args simulate TESTCALL 1234 60
```

Note: The .app bundle is required on macOS for Bluetooth permissions (TCC).
Output is also saved to `/tmp/ble_scan_output.txt`.

## What it shows
- Callsign (badge identity)
- RSSI (signal strength)
- Score (high score)
- Status (idle/playing/menu)
- Message ID (if sending a BLE comms message)
- Auth tag (validation hash)

## Integration Test (`test` mode)
Requires badge connected via USB serial. Tests:
1. Scans for badge advertisement, verifies it's visible
2. Changes callsign via serial, verifies advertisement updates
3. Starts game via serial, verifies status changes to "playing"
4. Checks BLE status via serial

## macOS Simulation Limitations
macOS CoreBluetooth **cannot** set manufacturer data in BLE advertisements.
Only service UUIDs and local name are supported. The badge requires valid
manufacturer data (callsign + score + auth tag) to accept a peer, so a
Mac cannot fully simulate a badge.

The `simulate` mode advertises the BSidesKC service UUID, which the badge's
scanner will detect, but the packet will be rejected at the manufacturer
data validation step (no callsign/tag present).

For full badge-to-badge simulation, use:
- A second ESP32 running badge firmware
- A Linux machine with BlueZ (supports raw HCI advertising)

## BLE Test Suite (Serial)

Comprehensive BLE testing via serial commands (no Bluetooth permissions needed):

```bash
python3 tools/ble-scanner/ble_test.py --port /dev/cu.usbserial-110
```

Tests: BLE status, crew roster, messaging, history, notifications, RSSI lookup,
BLE restart, callsign updates, BLE during game, rapid commands, concurrent
operations (bling+nav+BLE), game lifecycle with BLE, message stress, heap stability.

For visual BLE verification, run the Mac scanner in a separate terminal:

```bash
open tools/ble-scanner/BLEScan.app --args monitor
```