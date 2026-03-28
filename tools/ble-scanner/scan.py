#!/usr/bin/env python3
"""BSidesKC Badge BLE Scanner

Usage:
    # Via app bundle (required on macOS for Bluetooth permissions):
    open tools/ble-scanner/BLEScan.app

    # Scan for 10 seconds (default):
    open tools/ble-scanner/BLEScan.app --args scan

    # Scan for 30 seconds:
    open tools/ble-scanner/BLEScan.app --args scan 30

    # Continuous monitoring:
    open tools/ble-scanner/BLEScan.app --args monitor
"""
import asyncio
import sys
import time
from datetime import datetime

try:
    from bleak import BleakScanner
except ImportError:
    print('Install bleak: pip3 install bleak --break-system-packages')
    sys.exit(1)

BSIDES_UUID_FRAGMENT = 'bd26'


def parse_badge_data(device, ad):
    """Parse BSidesKC badge advertisement data."""
    for uuid in (ad.service_uuids or []):
        if BSIDES_UUID_FRAGMENT in uuid.lower():
            callsign = ''
            score = 0
            status = 0
            msg_id = 0
            tag = None
            for key, value in ad.manufacturer_data.items():
                raw = bytes(value)
                if len(raw) >= 10:
                    callsign = raw[:10].decode('utf-8', errors='replace').rstrip('\x00')
                if len(raw) >= 12:
                    score = raw[10] | (raw[11] << 8)
                if len(raw) >= 13:
                    status = raw[12]
                if len(raw) >= 14:
                    msg_id = raw[13]
                if len(raw) >= 16:
                    tag = raw[14] | (raw[15] << 8)
            return {
                'callsign': callsign,
                'score': score,
                'status': ['idle', 'playing', 'menu'][status] if status < 3 else f'unknown({status})',
                'msg_id': msg_id,
                'tag': tag,
                'rssi': ad.rssi,
                'address': device.address,
                'name': device.name,
            }
    return None


async def scan(duration=10):
    """Scan for badges for duration seconds."""
    print(f'Scanning for BSidesKC badges ({duration}s)...')
    badges = {}

    def callback(device, ad):
        badge = parse_badge_data(device, ad)
        if badge:
            addr = badge['address']
            is_new = addr not in badges
            badges[addr] = badge
            badges[addr]['last_seen'] = time.time()
            badges[addr]['count'] = badges.get(addr, {}).get('count', 0) + 1
            if is_new:
                print(f'  NEW: {badge["callsign"]} rssi={badge["rssi"]} score={badge["score"]} status={badge["status"]}')

    scanner = BleakScanner(callback)
    await scanner.start()
    await asyncio.sleep(duration)
    await scanner.stop()

    print(f'\n=== Found {len(badges)} badge(s) ===')
    for addr, b in badges.items():
        tag_str = f'  tag=0x{b["tag"]:04X}' if b.get('tag') is not None else '  tag=NONE'
        print(f'  {b["callsign"]:12s} rssi={b["rssi"]:4d}  score={b["score"]:5d}  status={b["status"]:8s}  seen={b["count"]}x{tag_str}  addr={addr}')
        if b['msg_id'] > 0:
            print(f'                 message_id={b["msg_id"]}')

    return badges


async def monitor():
    """Continuous monitoring - shows live updates."""
    print('Monitoring for BSidesKC badges (Ctrl+C to stop)...')
    badges = {}

    def callback(device, ad):
        badge = parse_badge_data(device, ad)
        if badge:
            addr = badge['address']
            is_new = addr not in badges
            old = badges.get(addr, {})
            badges[addr] = badge
            badges[addr]['last_seen'] = time.time()

            ts = datetime.now().strftime('%H:%M:%S')
            if is_new:
                print(f'[{ts}] NEW    {badge["callsign"]:12s} rssi={badge["rssi"]} score={badge["score"]} status={badge["status"]}')
            elif old.get('score') != badge['score']:
                print(f'[{ts}] SCORE  {badge["callsign"]:12s} {old.get("score",0)} -> {badge["score"]}')
            elif old.get('status') != badge['status']:
                print(f'[{ts}] STATUS {badge["callsign"]:12s} {old.get("status","?")} -> {badge["status"]}')
            elif old.get('msg_id', 0) != badge.get('msg_id', 0) and badge['msg_id'] > 0:
                print(f'[{ts}] MSG    {badge["callsign"]:12s} msg_id={badge["msg_id"]}')

    scanner = BleakScanner(callback)
    await scanner.start()
    try:
        while True:
            await asyncio.sleep(1)
    except KeyboardInterrupt:
        pass
    await scanner.stop()

    print(f'\nTotal badges seen: {len(badges)}')
    for addr, b in badges.items():
        print(f'  {b["callsign"]:12s} last_rssi={b["rssi"]} score={b["score"]}')


def main():
    args = sys.argv[1:]
    mode = args[0] if args else 'scan'

    if mode == 'scan':
        duration = int(args[1]) if len(args) > 1 else 10
        asyncio.run(scan(duration))
    elif mode == 'monitor':
        asyncio.run(monitor())
    else:
        print('Usage: scan [seconds] | monitor')


if __name__ == '__main__':
    main()
