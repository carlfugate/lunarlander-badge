# BSidesKC Badge BLE Scanner

Scan for BSidesKC badge BLE advertisements from macOS.

## Setup
```bash
pip3 install bleak --break-system-packages
```

## Usage
```bash
# Quick scan (10 seconds)
open tools/ble-scanner/BLEScan.app

# Scan for 30 seconds
open tools/ble-scanner/BLEScan.app --args scan 30

# Continuous monitoring (Ctrl+C to stop)
open tools/ble-scanner/BLEScan.app --args monitor
```

Note: The .app bundle is required on macOS for Bluetooth permissions (TCC).
Output is also saved to `/tmp/ble_scan_output.txt`.

## What it shows
- Callsign (badge identity)
- RSSI (signal strength)
- Score (high score)
- Status (idle/playing/menu)
- Message ID (if sending a BLE comms message)
