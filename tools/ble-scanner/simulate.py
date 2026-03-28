#!/usr/bin/env python3
"""Simulate a BSidesKC badge BLE advertisement from macOS.

Requires PyObjC (pip3 install pyobjc-framework-CoreBluetooth)
Must run via .app bundle for Bluetooth permissions.

Usage:
    open tools/ble-scanner/BLEScan.app --args simulate TESTMAC 1234
    # Advertises as callsign TESTMAC with score 1234

LIMITATION: macOS CoreBluetooth does NOT support setting manufacturer data
in BLE advertisements. Only service UUIDs and local name are allowed.
The badge filters by service UUID AND parses manufacturer data (callsign,
score, auth tag). Without manufacturer data, the badge will see the UUID
but reject the packet (no valid callsign/tag in manufacturer data).

For full badge simulation, use:
  - A second ESP32 running badge firmware
  - A Linux machine with BlueZ (supports raw HCI advertising)

This script advertises the service UUID so the badge's scan will detect
the advertisement, but it will be rejected at the manufacturer data
validation step in process_result().
"""
import sys
import time

try:
    import objc
    from Foundation import NSDate, NSRunLoop
    from CoreBluetooth import (CBPeripheralManager, CBAdvertisementDataServiceUUIDsKey,
                               CBAdvertisementDataLocalNameKey, CBPeripheralManagerDelegate)
    from CoreBluetooth import CBUUID
except ImportError:
    print('Install PyObjC: pip3 install pyobjc-framework-CoreBluetooth --break-system-packages')
    sys.exit(1)

BLE_AUTH_KEY = 0xB51D
BSIDES_UUID = CBUUID.UUIDWithString_('BD26')


def compute_tag(callsign, score):
    """Mirror firmware's compute_tag() for validation."""
    h = BLE_AUTH_KEY
    for i, c in enumerate(callsign[:10]):
        h ^= (ord(c) << (i % 8)) & 0xFFFF
        h = ((h << 3) | (h >> 13)) & 0xFFFF
    h ^= score & 0xFFFF
    h ^= (h >> 8) & 0xFFFF
    return h & 0xFFFF


class PeripheralDelegate(objc.lookUpClass('NSObject')):
    def peripheralManagerDidUpdateState_(self, peripheral):
        if peripheral.state() == 5:  # CBManagerStatePoweredOn
            print('Bluetooth powered on, starting advertisement...')
            ad_data = {
                CBAdvertisementDataLocalNameKey: 'BSidesKC',
                CBAdvertisementDataServiceUUIDsKey: [BSIDES_UUID],
            }
            peripheral.startAdvertising_(ad_data)

    def peripheralManagerDidStartAdvertising_error_(self, peripheral, error):
        if error:
            print(f'Advertising error: {error}')
        else:
            print('Advertising started (service UUID only, no manufacturer data)')


def simulate(callsign='SIMTEST', score=999, duration=60):
    tag = compute_tag(callsign, score)
    print(f'Simulating badge: callsign={callsign} score={score} tag=0x{tag:04X} duration={duration}s')
    print()
    print('WARNING: macOS CoreBluetooth cannot set manufacturer data.')
    print('Badge will see service UUID but reject packet (no callsign/tag).')
    print('For full simulation, use a second ESP32 or Linux with BlueZ.')
    print()

    delegate = PeripheralDelegate.alloc().init()
    manager = CBPeripheralManager.alloc().initWithDelegate_queue_(delegate, None)

    print(f'Running for {duration}s (Ctrl+C to stop)...')
    end = time.time() + duration
    try:
        while time.time() < end:
            NSRunLoop.currentRunLoop().runUntilDate_(NSDate.dateWithTimeIntervalSinceNow_(1.0))
    except KeyboardInterrupt:
        pass

    manager.stopAdvertising()
    print('Stopped.')


if __name__ == '__main__':
    callsign = sys.argv[1] if len(sys.argv) > 1 else 'SIMTEST'
    score = int(sys.argv[2]) if len(sys.argv) > 2 else 999
    duration = int(sys.argv[3]) if len(sys.argv) > 3 else 60
    simulate(callsign, score, duration)
