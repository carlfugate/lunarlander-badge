#!/bin/bash
RESOURCES="$(cd "$(dirname "$0")/../Resources" && pwd)"
python3 "$RESOURCES/scan.py" "$@" 2>&1 | tee /tmp/ble_scan_output.txt
