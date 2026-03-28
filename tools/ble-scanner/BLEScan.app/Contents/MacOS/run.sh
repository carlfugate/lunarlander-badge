#!/bin/bash
DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$DIR"
python3 scan.py "$@" 2>&1 | tee /tmp/ble_scan_output.txt
