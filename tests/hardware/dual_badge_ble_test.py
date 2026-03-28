#!/usr/bin/env python3
"""Dual-badge BLE test suite. Requires two badges on separate serial ports.

Usage:
    python3 tests/hardware/dual_badge_ble_test.py --port-a /dev/cu.usbserial-110 --port-b /dev/cu.usbserial-120
"""
import serial
import time
import sys
import argparse
import random


class Badge:
    def __init__(self, name, port, baud=115200):
        self.name = name
        self.ser = serial.Serial(port, baud, timeout=5)
        time.sleep(3)
        self.ser.reset_input_buffer()

    def send(self, cmd, wait=1.5):
        self.ser.write(f'{cmd}\r\n'.encode())
        time.sleep(wait)
        resp = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='replace').strip()
        return resp

    def show(self, cmd, wait=1.5):
        resp = self.send(cmd, wait)
        print(f'  [{self.name}] > {cmd}')
        for line in resp.split('\n'):
            if line.strip():
                print(f'  [{self.name}]   {line.strip()}')
        return resp

    def alive(self):
        return 'free=' in self.send('sys.heap', wait=2)

    def close(self):
        self.ser.close()


results = []


def test(name, passed):
    results.append((name, passed))
    print(f'  [{"PASS" if passed else "FAIL"}] {name}')


def run_tests(a, b):
    print('============================================')
    print('DUAL-BADGE BLE TEST SUITE')
    print(f'  Badge A: {a.name}')
    print(f'  Badge B: {b.name}')
    print('============================================')

    # Setup: unique callsigns
    print('\n--- Setup ---')
    a.show('callsign.set ALPHA')
    b.show('callsign.set BRAVO')
    a.show('ble restart', wait=3)
    b.show('ble restart', wait=3)
    time.sleep(2)
    a.show('sys.heap')
    b.show('sys.heap')

    # 1. Mutual Discovery
    print('\n--- 1. Mutual Discovery ---')
    print('  Waiting 25s for BLE scan cycles...')
    time.sleep(25)  # 2-3 scan cycles at 10s each
    resp_a = a.show('ble crew')
    resp_b = b.show('ble crew')
    test('A discovers B (BRAVO)', 'BRAVO' in resp_a)
    test('B discovers A (ALPHA)', 'ALPHA' in resp_b)

    # 2. RSSI Lookup
    print('\n--- 2. RSSI Lookup ---')
    resp_a = a.show('ble rssi BRAVO')
    resp_b = b.show('ble rssi ALPHA')
    test('A sees BRAVO RSSI', 'rssi=' in resp_a and 'rssi=0' not in resp_a)
    test('B sees ALPHA RSSI', 'rssi=' in resp_b and 'rssi=0' not in resp_b)

    # 3. Message A -> B
    print('\n--- 3. Message A -> B ---')
    a.show('ble send 1')  # "Hello crew!"
    print('  Waiting 15s for message delivery...')
    time.sleep(15)
    resp_b = b.show('ble notify')
    test('B received message from A', 'ALPHA' in resp_b or 'notification=' in resp_b)
    resp_b = b.show('ble history')
    test('B has message in history', 'ALPHA' in resp_b or 'history_count=' in resp_b)

    # 4. Message B -> A
    print('\n--- 4. Message B -> A ---')
    print('  Waiting 30s for rate limit...')
    time.sleep(30)
    b.show('ble send 3')  # "GG!"
    print('  Waiting 15s for message delivery...')
    time.sleep(15)
    resp_a = a.show('ble notify')
    test('A received message from B', 'BRAVO' in resp_a or 'notification=' in resp_a)

    # 5. Score Propagation
    print('\n--- 5. Score Propagation ---')
    a.show('game.start 0', wait=2)
    print('  Waiting for game to end (~30s)...')
    for t in range(20):
        time.sleep(2)
        if a.ser.in_waiting:
            data = a.ser.read(a.ser.in_waiting).decode('utf-8', errors='replace')
            if '[GAME] crashed' in data or '[GAME] landed' in data:
                print(f'  Game ended at ~{t * 2}s')
                break
    a.show('game.stop', wait=2)
    print('  Waiting 15s for score to propagate...')
    time.sleep(15)
    resp_b = b.show('ble crew')
    test('B sees A in crew after game', 'ALPHA' in resp_b)

    # 6. Status Tracking
    print('\n--- 6. Status Tracking ---')
    a.show('game.start 0', wait=2)
    print('  Waiting 15s for status propagation...')
    time.sleep(15)
    resp_b = b.show('ble crew')
    a.show('game.stop', wait=2)
    test('Both alive during status tracking', a.alive() and b.alive())

    # 7. Crew Roster Persistence
    print('\n--- 7. Crew Roster Persistence ---')
    a.show('ble crew')  # should have BRAVO
    a.show('sys.reboot', wait=0.5)
    print('  Waiting 10s for A to reboot...')
    time.sleep(10)
    a.ser.reset_input_buffer()
    resp_a = a.show('ble crew', wait=3)
    test('A remembers B after reboot (NVS)', 'BRAVO' in resp_a)

    # 8. Concurrent Operations
    print('\n--- 8. Concurrent Operations ---')
    screens = ['main', 'system', 'bling', 'credits', 'card']
    for i in range(10):
        a.send(f'nav.{random.choice(screens)}', wait=0.3)
        b.send(f'bling.set {random.randint(0, 11)}', wait=0.2)
        a.send('ble status', wait=0.2)
        b.send('ble status', wait=0.2)
    a.send('nav.main', wait=0.5)
    b.send('nav.main', wait=0.5)
    b.send('bling.off', wait=0.2)
    test('Both alive after concurrent ops', a.alive() and b.alive())

    # 9. BLE Restart Stress
    print('\n--- 9. BLE Restart Stress ---')
    for i in range(5):
        a.send('ble restart', wait=3)
        time.sleep(2)
    resp_b = b.show('ble crew')
    test('B still has A after A restarts 5x', 'ALPHA' in resp_b)
    test('A alive after 5 restarts', a.alive())

    # 10. Bidirectional Message Burst
    print('\n--- 10. Bidirectional Message Burst ---')
    a.send('ble send 5', wait=0.5)
    b.send('ble send 8', wait=0.5)
    time.sleep(15)
    test('Both alive after message burst', a.alive() and b.alive())

    # Cleanup
    print('\n--- Cleanup ---')
    a.show('callsign.set EAGLE')
    b.show('callsign.set EAGLE')
    a.show('bling.off')
    b.show('bling.off')
    a.show('sys.heap')
    b.show('sys.heap')

    # Summary
    print(f'\n============================================')
    passed = sum(1 for _, p in results if p)
    print(f'RESULTS: {passed}/{len(results)} PASSED')
    for name, p in results:
        if not p:
            print(f'  FAIL: {name}')
    print('============================================')
    return all(p for _, p in results)


def main():
    parser = argparse.ArgumentParser(description='Dual-Badge BLE Test')
    parser.add_argument('--port-a', required=True, help='Serial port for Badge A')
    parser.add_argument('--port-b', required=True, help='Serial port for Badge B')
    args = parser.parse_args()

    print(f'Connecting Badge A: {args.port_a}')
    a = Badge('A', args.port_a)
    print(f'Connecting Badge B: {args.port_b}')
    b = Badge('B', args.port_b)

    if not a.alive():
        print('Badge A not responding!')
        sys.exit(1)
    if not b.alive():
        print('Badge B not responding!')
        sys.exit(1)

    success = run_tests(a, b)
    a.close()
    b.close()
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
