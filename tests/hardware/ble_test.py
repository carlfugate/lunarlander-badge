#!/usr/bin/env python3
"""BLE test for BSidesKC badge. Scans for badge advertisements and verifies BLE is working."""
import asyncio
import sys
from bleak import BleakScanner

BSIDES_SERVICE_UUID = '0000bd26-0000-1000-8000-00805f9b34fb'  # 16-bit 0xBD26 as full UUID


async def scan_for_badge(duration=10):
    print(f'Scanning for BSidesKC badges ({duration}s)...')
    badges_found = []

    def callback(device, advertising_data):
        if advertising_data.service_uuids:
            for uuid in advertising_data.service_uuids:
                if 'bd26' in uuid.lower():
                    mfg = advertising_data.manufacturer_data
                    callsign = ''
                    score = 0
                    status = 0
                    for key, value in mfg.items():
                        if len(value) >= 13:
                            callsign = value[:10].decode('utf-8', errors='replace').rstrip('\x00')
                            score = value[10] | (value[11] << 8)
                            status = value[12]

                    info = {
                        'name': device.name,
                        'address': device.address,
                        'rssi': advertising_data.rssi,
                        'callsign': callsign,
                        'score': score,
                        'status': status,
                        'uuids': advertising_data.service_uuids,
                        'mfg_data': mfg,
                    }
                    if device.address not in [b['address'] for b in badges_found]:
                        badges_found.append(info)
                        print(f'  FOUND: {callsign} (RSSI:{advertising_data.rssi} Score:{score} Status:{status})')

    scanner = BleakScanner(callback)
    await scanner.start()
    await asyncio.sleep(duration)
    await scanner.stop()

    return badges_found


async def main():
    duration = int(sys.argv[1]) if len(sys.argv) > 1 else 10

    badges = await scan_for_badge(duration)

    print(f'\n=== Results ===')
    if badges:
        print(f'Found {len(badges)} badge(s):')
        for b in badges:
            print(f'  Callsign: {b["callsign"]}')
            print(f'  Address:  {b["address"]}')
            print(f'  RSSI:     {b["rssi"]} dBm')
            print(f'  Score:    {b["score"]}')
            print(f'  Status:   {b["status"]}')
            print(f'  UUIDs:    {b["uuids"]}')
            print(f'  Mfg Data: {b["mfg_data"]}')
            print()
    else:
        print('No BSidesKC badges found.')
        print('Make sure the badge is powered on and BLE is enabled.')

    return len(badges) > 0


if __name__ == '__main__':
    success = asyncio.run(main())
    sys.exit(0 if success else 1)
