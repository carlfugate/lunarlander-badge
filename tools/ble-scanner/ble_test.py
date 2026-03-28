#!/usr/bin/env python3
"""Comprehensive BLE test suite for BSidesKC badge.

Tests all BLE functions via serial commands. No Bluetooth permissions needed.
Run the Mac scanner separately for visual BLE advertisement verification.

Usage:
    python3 tools/ble-scanner/ble_test.py [--port /dev/cu.usbserial-110]
"""
import serial
import time
import random
import sys
import argparse

DEFAULT_PORT = '/dev/cu.usbserial-110'
DEFAULT_BAUD = 115200

results = []


def connect(port, baud=DEFAULT_BAUD):
    s = serial.Serial(port, baud, timeout=3)
    time.sleep(3)
    s.reset_input_buffer()
    return s


def send(s, cmd, wait=1.5):
    s.reset_input_buffer()
    s.write(f'{cmd}\r\n'.encode())
    time.sleep(wait)
    return s.read(s.in_waiting).decode('utf-8', errors='replace').strip()


def show(s, cmd, wait=1.5):
    resp = send(s, cmd, wait)
    print(f'  > {cmd}')
    for line in resp.split('\n'):
        if line.strip():
            print(f'    {line.strip()}')
    return resp


def alive(s):
    return 'free=' in send(s, 'sys.heap', wait=2)


def test(name, passed):
    results.append((name, passed))
    print(f'  [{"PASS" if passed else "FAIL"}] {name}')


def get_heap(s):
    resp = send(s, 'sys.heap', wait=2)
    for part in resp.split():
        if part.startswith('free='):
            try:
                return int(part.split('=')[1])
            except ValueError:
                pass
    return 0


# === Test Groups ===

def test_ble_status(s):
    print('--- 1. BLE Status ---')
    resp = show(s, 'ble status')
    test('ble.status responds', '[BLE]' in resp)
    test('has nearby count', 'nearby=' in resp)
    test('has total count', 'total=' in resp)


def test_crew_roster(s):
    print('\n--- 2. Crew Roster ---')
    resp = show(s, 'ble crew')
    test('ble.crew responds', 'crew_count=' in resp)


def test_preset_messages(s):
    print('\n--- 3. Preset Messages ---')
    resp = show(s, 'ble messages')
    test('ble.messages lists presets', 'preset_messages=' in resp)


def test_send_message(s):
    print('\n--- 4. Send Message ---')
    resp = show(s, 'ble send 1')
    test('ble.send responds', 'sent' in resp or 'BLE' in resp)
    # Second send — may be rate-limited (30s cooldown)
    resp2 = show(s, 'ble send 2', wait=0.5)
    test('badge alive after double send', alive(s))


def test_message_history(s):
    print('\n--- 5. Message History ---')
    resp = show(s, 'ble history')
    test('ble.history responds', 'history_count=' in resp)


def test_notification(s):
    print('\n--- 6. Notification Check ---')
    resp = show(s, 'ble notify')
    test('ble.notify responds', 'notification=' in resp)


def test_rssi_lookup(s):
    print('\n--- 7. RSSI Lookup ---')
    resp = show(s, 'ble rssi NOBODY')
    test('ble.rssi unknown returns not_found', 'rssi=0' in resp or 'not_found' in resp)


def test_ble_restart(s):
    print('\n--- 8. BLE Restart ---')
    resp = show(s, 'ble restart', wait=3)
    test('ble.restart responds', 'restarted' in resp)
    time.sleep(2)
    resp = show(s, 'ble status')
    test('ble works after restart', 'nearby=' in resp)


def test_callsign_update(s):
    print('\n--- 9. Callsign Update + BLE ---')
    show(s, 'callsign.set BLETEST1', wait=1)
    time.sleep(1)
    resp = show(s, 'callsign.get')
    test('callsign changed', 'BLETEST1' in resp)
    resp = show(s, 'ble status')
    test('ble ok after callsign change', 'nearby=' in resp)
    show(s, 'callsign.set EAGLE', wait=1)


def test_ble_during_game(s):
    print('\n--- 10. BLE During Game ---')
    show(s, 'game.start 0', wait=2)
    resp = show(s, 'ble status')
    test('ble.status during game', 'nearby=' in resp)
    resp = show(s, 'ble crew')
    test('ble.crew during game', 'crew_count=' in resp)
    show(s, 'game.stop', wait=2)
    test('badge alive after game+ble', alive(s))


def test_rapid_commands(s):
    print('\n--- 11. Rapid BLE Commands (x20) ---')
    cmds = ['ble status', 'ble crew', 'ble history', 'ble notify']
    for _ in range(20):
        send(s, random.choice(cmds), wait=0.2)
    test('badge alive after 20 rapid ble cmds', alive(s))


def test_concurrent_subsystems(s):
    print('\n--- 12. BLE + Bling + Nav Concurrent ---')
    send(s, 'bling.set 1', wait=0.3)
    navs = ['nav.main', 'nav.system', 'nav.bling', 'nav.credits', 'nav.card']
    for _ in range(10):
        send(s, random.choice(navs), wait=0.3)
        send(s, 'ble status', wait=0.2)
    send(s, 'nav.main', wait=0.5)
    send(s, 'bling.off', wait=0.3)
    test('badge alive after ble+bling+nav', alive(s))


def test_game_lifecycle(s):
    print('\n--- 13. BLE + Game Lifecycle x5 ---')
    for _ in range(5):
        send(s, 'game.start 0', wait=0.5)
        send(s, 'ble status', wait=0.3)
        send(s, 'game.stop', wait=0.5)
        send(s, 'ble status', wait=0.3)
    test('badge alive after 5 game cycles with ble', alive(s))


def test_send_stress(s):
    print('\n--- 14. Message Send Stress (x12) ---')
    for i in range(12):
        send(s, f'ble send {(i % 12) + 1}', wait=0.3)
    test('badge alive after 12 rapid sends', alive(s))


def test_restart_under_load(s):
    print('\n--- 15. BLE Restart Under Load ---')
    send(s, 'bling.set 4', wait=0.3)
    send(s, 'ble restart', wait=3)
    resp = send(s, 'ble status', wait=2)
    send(s, 'bling.off', wait=0.3)
    test('ble works after restart under load', 'nearby=' in resp)


def test_heap_stability(s, heap_before):
    print('\n--- 16. Heap Stability ---')
    heap_after = get_heap(s)
    leak = heap_before - heap_after
    show(s, 'sys.heap')
    test(f'heap check (before={heap_before} after={heap_after} delta={leak})', heap_after > 0)
    test('no major heap leak (<5KB)', leak < 5000)


# === Main ===

def run_tests(s):
    print('=== BLE COMPREHENSIVE TEST SUITE ===\n')

    heap_before = get_heap(s)
    print(f'Initial heap: {heap_before}\n')

    test_ble_status(s)
    test_crew_roster(s)
    test_preset_messages(s)
    test_send_message(s)
    test_message_history(s)
    test_notification(s)
    test_rssi_lookup(s)
    test_ble_restart(s)
    test_callsign_update(s)
    test_ble_during_game(s)
    test_rapid_commands(s)
    test_concurrent_subsystems(s)
    test_game_lifecycle(s)
    test_send_stress(s)
    test_restart_under_load(s)
    test_heap_stability(s, heap_before)

    passed = sum(1 for _, p in results if p)
    total = len(results)
    print(f'\n=== RESULTS: {passed}/{total} PASSED ===')
    for name, p in results:
        if not p:
            print(f'  FAIL: {name}')

    return passed == total


def main():
    parser = argparse.ArgumentParser(description='BLE Test Suite for BSidesKC Badge')
    parser.add_argument('--port', default=DEFAULT_PORT, help='Serial port')
    parser.add_argument('--baud', type=int, default=DEFAULT_BAUD, help='Baud rate')
    args = parser.parse_args()

    print(f'Connecting to {args.port}...')
    s = connect(args.port, args.baud)

    if not alive(s):
        print('Badge not responding!')
        s.close()
        sys.exit(1)

    print('Badge connected.\n')
    success = run_tests(s)
    s.close()
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
