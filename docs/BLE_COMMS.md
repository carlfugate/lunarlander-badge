# BLE Communications — BSidesKC 2026 Badge

## Architecture

### Overview
Badges communicate via BLE advertising only — no pairing, no GATT connections. Each badge broadcasts its identity and state, and passively scans for other badges.

### Stack
- NimBLE-Arduino (replaced Bluedroid to save ~100KB RAM)
- Advertising + passive scanning only
- No GATT server/client
- 10-second scan interval via LVGL timer

### Advertisement Payload (18 bytes)
```
Bytes 0-1:   Company ID (0xFFFF — BLE SIG testing)
Bytes 2-11:  Callsign (10 bytes, null-padded ASCII)
Bytes 12-13: High score (uint16_t, little-endian)
Bytes 14:    Status (0=idle, 1=playing, 2=menu)
Bytes 15:    Message ID (0=none, 1-12=preset message)
Bytes 16-17: Auth tag (uint16_t, shared secret hash)
```

Service UUID: 0xBD26 (16-bit, "BD" badge + "26" for 2026)

### Security
- **Auth tag**: 2-byte hash of callsign + score + conference key (BLE_AUTH_KEY)
- **Input validation**: printable ASCII callsign, score 0-10000, msg_id 0-12
- **Flood protection**: max 3 new discoveries per 5 seconds
- **FF_BLE_NO_AUTH**: compile-time flag to disable auth for testing
- **No GATT**: no connection-based attack surface

### Features Implemented
1. **Presence broadcast** — advertise callsign + score + status
2. **Passive scanning** — discover nearby badges every 10s
3. **Crew roster** — store up to 100 discovered badges with first-seen MET, RSSI
4. **Nearby count** — badges seen in last 30s shown on main menu data panel
5. **Proximity greeting** — cyan LED pulse on first discovery (deferred to main task)
6. **Preset messages** — 12 space/con-themed messages, 30s rate limit
7. **Message notification** — purple LED pulse + sender callsign on data panel
8. **Crew log screen** — scrollable list of all encountered badges
9. **Comms screen** — 2x6 grid of preset message buttons with rate limit indicator

### Preset Messages
| ID | Message |
|----|---------|
| 1 | Hello crew! |
| 2 | Nice badge! |
| 3 | GG! |
| 4 | Need coffee |
| 5 | CTF team up? |
| 6 | Good talk! |
| 7 | After party? |
| 8 | Ad astra! |
| 9 | SOS - need help |
| 10 | Check the leaderboard |
| 11 | Lander challenge? |
| 12 | See you at the bar |

### Known Issues
- **#94**: Advertisement not refreshed after callsign/score/status changes
- BLE disabled during game was removed (NimBLE fits in memory)
- Min heap during game with BLE: ~25KB (tight but functional)
- Crew roster not persisted to SD (resets on reboot)

---

## Files
| File | Purpose |
|------|---------|
| `include/QA/BlePresence.h` | API declarations, CrewEntry struct |
| `src/QA/BlePresence.cpp` | Full implementation + NATIVE_TEST stubs |
| `include/FeatureFlags.h` | FF_BLE_NO_AUTH flag |
| `tools/ble-scanner/` | macOS scanner + integration test |

---

## API Reference

### Lifecycle
```cpp
void ble_presence_init(const char *my_callsign, uint16_t my_score);
void ble_presence_stop();      // deinit BLE entirely
void ble_presence_restart();   // re-init after stop
```

### State Updates
```cpp
void ble_presence_update_score(uint16_t score);  // updates adv immediately
void ble_presence_set_status(uint8_t status);     // 0=idle, 1=playing, 2=menu
void ble_presence_send_message(uint8_t msg_id);   // 1-12, 30s rate limit
```

### Queries
```cpp
int ble_presence_nearby_count();    // badges seen in last 30s
int ble_presence_total_count();     // total unique badges ever seen
const CrewEntry* ble_presence_get_crew();
int ble_presence_get_crew_count();
const char* ble_presence_get_message_text(uint8_t msg_id);
```

### Notifications
```cpp
bool ble_presence_has_notification();    // true if unread message
const char* ble_presence_get_notification();  // "CALLSIGN: message text"
void ble_presence_clear_notification();
```

### UI Screens
```cpp
void create_comms_window();      // 2x6 message button grid
void create_crew_log_window();   // scrollable crew roster
```

---

## Testing Tools

### Mac BLE Scanner
```bash
# Quick scan (10 seconds)
open tools/ble-scanner/BLEScan.app

# Scan for 30 seconds
open tools/ble-scanner/BLEScan.app --args scan 30

# Continuous monitoring (Ctrl+C to stop)
open tools/ble-scanner/BLEScan.app --args monitor

# Integration test (scan + serial verification)
open tools/ble-scanner/BLEScan.app --args test /dev/cu.usbserial-110
```

Output saved to `/tmp/ble_scan_output.txt`.

### Serial Commands
| Command | Description |
|---------|-------------|
| `ble.status` | Show BLE state, nearby count, crew count |
| `ble.send <n>` | Send preset message (1-12) |
| `ble.crew` | List all discovered crew |
| `ble.auth` | Show current auth tag |

### Compile-Time Flags
- `FF_BLE_NO_AUTH` — disable auth tag verification (for Mac scanner testing)

### Limitations
- macOS CoreBluetooth cannot set manufacturer data in advertisements — Mac can scan but not fully simulate a badge
- Full badge-to-badge testing requires two ESP32 devices or Linux with BlueZ
- BLE advertising is one-way — no acknowledgment that a message was received

---

## Memory Profile (NimBLE)

| Stage | Free Heap | Max Block |
|-------|-----------|-----------|
| Boot | ~280 KB | ~200 KB |
| After NimBLE init | ~250 KB | ~172 KB |
| Game canvas allocated | ~97 KB | ~25 KB |
| Min during game+BLE | ~25 KB | — |

See `docs/BLE_MEMORY_PLAN.md` for the full Bluedroid→NimBLE migration analysis.

---

## Test Results (as of March 28, 2026)
- Mac BLE scan: detects badge, parses callsign/score/status/tag correctly
- 500-op stress test with BLE active: PASS, no hangs
- 8-hour overnight stress (without BLE): PASS, 9352 ops, 0 leaks
- BLE + game concurrent: PASS

---

## Feature Roadmap

### Must-Have (before conference)
| Feature | Issue | Depends On |
|---------|-------|------------|
| Fix advertisement refresh on state changes | #94 | — |
| Persist crew roster to SD card | #95 | — |
| Message received history/log | #96 | — |
| Update BLE status on game start/stop | #94 | #94 |
| Broadcast high score updates | #97 | #94 |

### Should-Have
| Feature | Issue | Depends On |
|---------|-------|------------|
| "Find my friend" RSSI indicator | #98 | — |
| Crew roster stats on main menu ("Met 23 crew") | — | #95 |
| Score taunt — broadcast score for 30s after landing | #97 | #94 |

### Nice-to-Have
| Feature | Notes |
|---------|-------|
| Custom message entry | Requires expanding payload or GATT |
| Badge-to-badge game challenge | Broadcast challenge seed, both start same game |
| Crew roster achievements | "Met 10 crew", "Met 50 crew" |

### Testing Infrastructure
| Feature | Issue | Notes |
|---------|-------|-------|
| Mac/Linux BLE advertiser | #99 | Needed for receive-side testing |
| BLE stress test suite | #100 | Flood, rate limit, concurrent load |
