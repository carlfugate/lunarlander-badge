#!/usr/bin/env python3
"""BSidesKC Badge Overnight Soak Test

Runs for a configurable duration (default 8 hours), exercising every badge
function through random and structured navigation paths. Detects crashes,
hangs, and heap leaks. Self-recovers from failures.

Usage:
    python3 tests/hardware/overnight_soak.py [--hours 8] [--port /dev/cu.usbserial-10]
"""
import serial
import time
import random
import sys
import os
import argparse
from datetime import datetime, timedelta

# === Configuration ===
DEFAULT_PORT = '/dev/cu.usbserial-10'
DEFAULT_BAUD = 115200
DEFAULT_HOURS = 8
COMMAND_TIMEOUT = 5       # seconds to wait for response
HEAP_LEAK_THRESHOLD = 5000  # bytes lost = potential leak
LOG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'logs')

# All navigable screens (from nav_handler in Menu.cpp)
SCREENS = [
    'main', 'system', 'battery', 'buzzer', 'sd', 'info', 'ota',
    'credits', 'card', 'screensaver', 'checkin', 'bling', 'wifi',
    'callsign', 'achievements', 'crew', 'comms', 'schedule',
]

# Bling modes 0-11 (from bling list command)
BLING_MODES = list(range(12))

# Screensaver modes 0-3 (ad_astra, matrix, terminal, lava)
SS_MODES = [0, 1, 2, 3]

# BLE preset message IDs
BLE_MESSAGES = list(range(1, 13))

# Callsigns to cycle
CALLSIGNS = ['EAGLE', 'COLUMBIA', 'ODYSSEY', 'PHOENIX', 'ORION', 'GEMINI']


class BadgeConnection:
    """Manages serial connection to badge with reconnect capability."""

    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.ser = None
        self.connect()

    def connect(self):
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except Exception:
            pass
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=COMMAND_TIMEOUT)
            time.sleep(2)
            self.ser.reset_input_buffer()
            return True
        except Exception:
            return False

    def send(self, cmd, timeout=COMMAND_TIMEOUT):
        """Send command, return (success, response). Detects serial errors."""
        try:
            self.ser.reset_input_buffer()
            self.ser.write(f'{cmd}\n'.encode())
            self.ser.flush()

            lines = []
            deadline = time.time() + timeout
            while time.time() < deadline:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='replace').strip()
                    if line:
                        lines.append(line)
                        # Got a tagged response — grab trailing lines briefly
                        if line.startswith('['):
                            deadline = min(deadline, time.time() + 0.3)
                else:
                    time.sleep(0.05)

            return (True, '\n'.join(lines))
        except serial.SerialException:
            return (False, 'SERIAL_ERROR')
        except Exception as e:
            return (False, str(e))

    def get_heap(self):
        """Return (free, min) heap or None on failure."""
        ok, resp = self.send('sys.heap')
        if ok and 'free=' in resp:
            try:
                free = int(resp.split('free=')[1].split()[0])
                min_heap = int(resp.split('min=')[1].split()[0])
                return (free, min_heap)
            except (ValueError, IndexError):
                pass
        return None

    def is_alive(self):
        ok, resp = self.send('sys.uptime', timeout=3)
        return ok and 'ms=' in resp

    def wait_for_boot(self, max_wait=30):
        """Wait for badge to come back after crash/reboot."""
        deadline = time.time() + max_wait
        while time.time() < deadline:
            time.sleep(2)
            try:
                self.connect()
                if self.is_alive():
                    return True
            except Exception:
                pass
        return False


class SoakTest:
    def __init__(self, badge, hours, log_file):
        self.badge = badge
        self.duration = timedelta(hours=hours)
        self.log_file = log_file
        self.start_time = None
        self.stats = {
            'commands_sent': 0,
            'commands_ok': 0,
            'commands_failed': 0,
            'crashes': 0,
            'hangs': 0,
            'recoveries': 0,
            'heap_warnings': 0,
            'cycles': 0,
        }
        self.heap_history = []

    def log(self, level, msg):
        ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        elapsed = str(datetime.now() - self.start_time).split('.')[0] if self.start_time else '0:00:00'
        line = f'[{ts}] [{elapsed}] [{level}] {msg}'
        print(line)
        with open(self.log_file, 'a') as f:
            f.write(line + '\n')

    def cmd(self, command, timeout=COMMAND_TIMEOUT):
        """Send command with crash/hang detection and auto-recovery."""
        self.stats['commands_sent'] += 1
        ok, resp = self.badge.send(command, timeout=timeout)

        # Serial error — reconnect
        if not ok or resp == 'SERIAL_ERROR':
            self.log('ERROR', f'Serial error on: {command}')
            self.stats['commands_failed'] += 1
            self._recover('serial_error')
            return None

        # Crash detection (Guru Meditation, assert, reset vector)
        if 'Guru Meditation' in resp or 'assert failed' in resp or 'rst:' in resp:
            self.log('CRASH', f'cmd={command} resp={resp[:200]}')
            self.stats['crashes'] += 1
            self._recover('crash')
            return None

        # Hang detection — no response to a command that should produce one
        if not resp and command not in ('bling.off',):
            if not self.badge.is_alive():
                self.log('HANG', f'No response after: {command}')
                self.stats['hangs'] += 1
                self._recover('hang')
                return None

        self.stats['commands_ok'] += 1
        return resp

    def _recover(self, reason):
        """Reconnect and return badge to known state."""
        self.log('RECOVER', f'Recovery from {reason}...')
        for attempt in range(1, 4):
            self.log('RECOVER', f'Attempt {attempt}/3')
            if self.badge.wait_for_boot(max_wait=20):
                self.log('RECOVER', 'Badge alive — resetting to main')
                self.stats['recoveries'] += 1
                self.badge.send('nav.main')
                time.sleep(1)
                return True
        self.log('FATAL', f'Could not recover from {reason}')
        return False

    def check_heap(self, context=''):
        """Record heap snapshot and flag leaks."""
        heap = self.badge.get_heap()
        if not heap:
            return None
        free, min_free = heap
        self.heap_history.append((datetime.now(), free, min_free, context))
        if len(self.heap_history) > 1:
            initial = self.heap_history[0][1]
            if free - initial < -HEAP_LEAK_THRESHOLD:
                self.log('LEAK', f'Heap dropped {initial - free}B: {initial}->{free} ({context})')
                self.stats['heap_warnings'] += 1
        return heap

    # === Test Scenarios ===

    def test_sequential_nav(self):
        """Visit every screen in order."""
        self.log('TEST', 'Sequential navigation')
        for s in SCREENS:
            self.cmd(f'nav.{s}')
            time.sleep(0.3)
        self.cmd('nav.main')

    def test_random_nav(self):
        """Navigate to 20 random screens."""
        self.log('TEST', 'Random navigation x20')
        for _ in range(20):
            self.cmd(f'nav.{random.choice(SCREENS)}')
            time.sleep(0.3)
        self.cmd('nav.main')

    def test_rapid_nav(self):
        """50 rapid screen switches, no delay."""
        self.log('TEST', 'Rapid navigation x50')
        for _ in range(50):
            self.cmd(f'nav.{random.choice(SCREENS)}')
        self.cmd('nav.main')

    def test_deep_nav(self):
        """Structured deep navigation paths."""
        paths = [
            ['system', 'battery', 'system', 'buzzer', 'system', 'info', 'main'],
            ['bling', 'main', 'screensaver', 'main', 'callsign', 'main'],
            ['achievements', 'main', 'crew', 'main', 'comms', 'main'],
            ['credits', 'main', 'card', 'main', 'schedule', 'main'],
            ['wifi', 'main', 'checkin', 'main', 'sd', 'main'],
        ]
        path = random.choice(paths)
        self.log('TEST', f'Deep nav: {" -> ".join(path)}')
        for s in path:
            self.cmd(f'nav.{s}')
            time.sleep(0.3)

    def test_bling_cycle(self):
        """Cycle through all 12 bling modes."""
        self.log('TEST', 'Bling cycle: all modes')
        for mode in BLING_MODES:
            self.cmd(f'bling.set {mode}')
            time.sleep(0.5)
        self.cmd('bling.off')

    def test_bling_rapid(self):
        """30 rapid bling mode switches."""
        self.log('TEST', 'Bling rapid x30')
        for _ in range(30):
            self.cmd(f'bling.set {random.choice(BLING_MODES)}')
        self.cmd('bling.off')

    def test_screensaver_modes(self):
        """Cycle all screensaver modes."""
        self.log('TEST', 'Screensaver mode cycle')
        for m in SS_MODES:
            self.cmd(f'screensaver.mode {m}')
            time.sleep(0.5)
        self.cmd('screensaver.mode 0')

    def test_callsign_cycle(self):
        """Set and verify callsigns."""
        self.log('TEST', 'Callsign cycle')
        for name in CALLSIGNS:
            self.cmd(f'callsign.set {name}')
            self.cmd('callsign.get')
            time.sleep(0.3)

    def test_audio_toggle(self):
        """Toggle mute 10 times."""
        self.log('TEST', 'Audio toggle x10')
        for _ in range(10):
            self.cmd('audio.mute')
            self.cmd('audio.unmute')
        self.cmd('audio.unmute')

    def test_ble(self):
        """BLE status and send a message."""
        self.log('TEST', 'BLE operations')
        self.cmd('ble.status')
        self.cmd(f'ble.send {random.choice(BLE_MESSAGES)}')
        self.cmd('ble.crew')

    def test_achievements(self):
        """Query achievements."""
        self.log('TEST', 'Achievements check')
        self.cmd('achievements.status')
        self.cmd('achievements.list')

    def test_mixed_ops(self):
        """Random mix of 30 operations."""
        self.log('TEST', 'Mixed operations x30')
        ops = [
            lambda: self.cmd(f'nav.{random.choice(SCREENS)}'),
            lambda: self.cmd(f'bling.set {random.choice(BLING_MODES)}'),
            lambda: self.cmd('bling.off'),
            lambda: (self.cmd('audio.mute'), self.cmd('audio.unmute')),
            lambda: self.cmd('sys.heap'),
            lambda: self.cmd('ble.status'),
            lambda: self.cmd('achievements.status'),
            lambda: self.cmd(f'screensaver.mode {random.choice(SS_MODES)}'),
            lambda: self.cmd(f'callsign.set {random.choice(CALLSIGNS)}'),
        ]
        for _ in range(30):
            random.choice(ops)()
            time.sleep(0.2)
        self.cmd('nav.main')
        self.cmd('bling.off')

    def test_screensaver_soak(self):
        """Trigger screensaver in each mode, let it run, then wake."""
        self.log('TEST', 'Screensaver soak: all modes')
        for mode in SS_MODES:
            self.cmd(f'screensaver.mode {mode}')
            self.cmd('nav.main')  # ensure we're on main for idle timer
            self.log('TEST', f'Screensaver mode {mode}: idling 90s...')
            heap_before = self.badge.get_heap()
            self.cmd('test.idle 90000', timeout=100)  # 90s triggers screensaver + 30s of scene
            self.cmd('nav.main')  # wake
            heap_after = self.badge.get_heap()
            if heap_before and heap_after:
                delta = heap_after[0] - heap_before[0]
                self.log('TEST', f'Screensaver mode {mode}: heap delta={delta}')
        self.cmd('screensaver.mode 0')  # restore default

    # === Main Loop ===

    def run(self):
        self.start_time = datetime.now()
        end_time = self.start_time + self.duration

        self.log('START', f'Duration: {self.duration} | End: {end_time.strftime("%Y-%m-%d %H:%M:%S")}')

        # Baseline
        self.cmd('nav.main')
        initial_heap = self.check_heap('initial')
        self.log('INFO', f'Initial heap: {initial_heap}')

        # Weighted test pool
        weighted = []
        for fn, w in [
            (self.test_sequential_nav, 1),
            (self.test_random_nav, 2),
            (self.test_rapid_nav, 1),
            (self.test_deep_nav, 3),
            (self.test_bling_cycle, 1),
            (self.test_bling_rapid, 1),
            (self.test_screensaver_modes, 1),
            (self.test_callsign_cycle, 1),
            (self.test_audio_toggle, 1),
            (self.test_ble, 1),
            (self.test_achievements, 1),
            (self.test_mixed_ops, 2),
            (self.test_screensaver_soak, 1),
        ]:
            weighted.extend([fn] * w)

        cycle = 0
        while datetime.now() < end_time:
            cycle += 1
            self.stats['cycles'] = cycle
            elapsed = str(datetime.now() - self.start_time).split('.')[0]
            self.log('CYCLE', f'=== Cycle {cycle} ({elapsed} elapsed) ===')

            # 3-6 random tests per cycle
            for fn in random.sample(weighted, min(random.randint(3, 6), len(weighted))):
                if datetime.now() >= end_time:
                    break
                try:
                    fn()
                except Exception as e:
                    self.log('ERROR', f'Test exception: {e}')

            # Heap check every cycle
            heap = self.check_heap(f'cycle_{cycle}')
            if heap:
                self.log('HEAP', f'free={heap[0]} min={heap[1]}')

            # Idle soak every 10 cycles (90s triggers screensaver + one scene transition)
            if cycle % 10 == 0:
                self.log('SOAK', 'Idle soak 90s')
                self.cmd('test.idle 90000', timeout=100)

            # Every 50 cycles, do a long idle soak (5 min) to test screensaver stability
            if cycle % 50 == 0:
                self.log('SOAK', f'Long idle soak at cycle {cycle} (300s)')
                heap_before = self.check_heap(f'long_soak_start_{cycle}')
                self.cmd('test.idle 300000', timeout=310)
                heap_after = self.check_heap(f'long_soak_end_{cycle}')
                if heap_before and heap_after:
                    delta = heap_after[0] - heap_before[0]
                    self.log('SOAK', f'Long soak heap delta: {delta} bytes')

            # Stress test every 20 cycles
            if cycle % 20 == 0:
                self.log('STRESS', 'Stress test 20 cycles')
                self.cmd('test.stress 20', timeout=30)

            # Reset to known state
            self.cmd('nav.main')
            self.cmd('bling.off')
            self.cmd('audio.unmute')

        # === Final Report ===
        self.log('END', '=== SOAK TEST COMPLETE ===')
        final_heap = self.check_heap('final')
        total_time = str(datetime.now() - self.start_time).split('.')[0]

        for k, v in self.stats.items():
            self.log('REPORT', f'{k}: {v}')

        if initial_heap and final_heap:
            delta = final_heap[0] - initial_heap[0]
            self.log('REPORT', f'heap_delta: {delta}B (initial={initial_heap[0]} final={final_heap[0]})')
            self.log('REPORT', f'min_heap_ever: {final_heap[1]}')

        passed = (self.stats['crashes'] == 0 and
                  self.stats['hangs'] == 0 and
                  self.stats['heap_warnings'] == 0)
        self.log('VERDICT', 'PASS' if passed else 'FAIL')
        return passed


def main():
    parser = argparse.ArgumentParser(description='BSidesKC Badge Overnight Soak Test')
    parser.add_argument('--hours', type=float, default=DEFAULT_HOURS)
    parser.add_argument('--port', default=DEFAULT_PORT)
    parser.add_argument('--baud', type=int, default=DEFAULT_BAUD)
    args = parser.parse_args()

    os.makedirs(LOG_DIR, exist_ok=True)
    log_file = os.path.join(LOG_DIR, f'soak_{datetime.now().strftime("%Y%m%d_%H%M%S")}.log')

    print(f'BSidesKC Badge Overnight Soak Test')
    print(f'Port: {args.port} | Duration: {args.hours}h | Log: {log_file}\n')

    badge = BadgeConnection(args.port, args.baud)
    if not badge.is_alive():
        print('ERROR: Badge not responding. Check connection.')
        sys.exit(1)

    soak = SoakTest(badge, args.hours, log_file)
    sys.exit(0 if soak.run() else 1)


if __name__ == '__main__':
    main()
