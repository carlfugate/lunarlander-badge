# BLE + Game Canvas Memory Plan

## Hardware Constraints

ESP32-S3-N8: 512KB SRAM total (shared IRAM/DRAM), no PSRAM, 8MB flash.

## Measured Memory Timeline (from boot logs)

| Stage | Free Heap | Max Block | Notes |
|-------|-----------|-----------|-------|
| Boot start | 216 KB | 180 KB | Before any init |
| After LVGL + display + touch | 213 KB | 180 KB | 3KB used, no fragmentation |
| After BLE init (Bluedroid) | 178 KB | 139 KB | 35KB used, **fragmented** |
| Canvas allocation needs | — | **153 KB** | 320×240×2 RGB565 |

**The problem:** BLE fragments the heap. 178KB total free but only 139KB contiguous.
Canvas needs 153KB contiguous. Gap: **14KB short**.

---

## Linker Map Analysis

From `.pio/build/esp32-s3/firmware.map`:

```
DRAM segment:  0x3fc88000, length 337KB (345,856 bytes)
IRAM segment:  0x40374000, length 353KB (362,240 bytes)

Static data+bss:  149,548 bytes (146KB) — 45.6% of 327,680
IRAM used:         71,148 bytes (69KB)  — 19.6% of 362,240

Heap start:  0x3fcb9e20 (after .bss end)
```

### BT/Bluedroid Static Sections (baked into firmware)

```
_bt_data_start:     0x3fc99a60
_nimble_data_start: 0x3fc99fa4
_bt_bss_end:        0x3fcb7514
_btdm_bss_end:      0x3fcb9e1c

Total BT static:    128KB (132,028 bytes)
```

These 128KB are part of the 146KB static data+bss. The Arduino ESP32 SDK is
pre-compiled with `CONFIG_BT_ENABLED=y` and `CONFIG_BT_BLUEDROID_ENABLED=y`.
Classic BT sections are released to heap at boot (explaining why boot heap is
216KB despite only ~174KB DRAM heap space). BLE controller sections remain reserved.

### Current BLE Library

Using **ESP32 BLE Arduino** (Bluedroid-based), the built-in Arduino framework library.
Not NimBLE. This is the most memory-hungry option.

SDK config confirms: `CONFIG_BT_BLUEDROID_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED` is NOT set.

---

## Solution Evaluation

### Option A: Static Canvas Buffer (.bss)

Declare canvas as a global array so the linker places it contiguously at compile time.

```cpp
static uint8_t __attribute__((aligned(4))) canvas_buf[320*240*2]; // 153,600 bytes in .bss
```

**Memory math:**
- Current static: 149,548 bytes
- Plus canvas: +153,600 bytes = 303,148 bytes total
- DRAM capacity (PIO reports): 327,680 bytes
- Remaining for heap: **24,532 bytes (24KB)**
- Plus extra heap regions (released BT classic, IRAM): ~42KB
- Total boot heap: **~66KB**
- BLE dynamic needs: ~35-40KB
- Remaining after BLE: **~26KB**

**Verdict: RISKY.** Fits in DRAM segment but leaves only ~26KB for LVGL, WiFi,
string allocations, FreeRTOS stacks, etc. WiFi cannot run simultaneously.
Any future feature additions would be impossible.

| Metric | Rating |
|--------|--------|
| Feasibility | Compiles, but dangerously tight |
| Memory savings | N/A (moves allocation from heap to .bss) |
| Complexity | Trivial (one line change) |
| Visual/UX impact | None |
| Risk | High — no headroom for WiFi, OTA, or growth |

### Option B: IRAM for Canvas

Use `heap_caps_malloc(size, MALLOC_CAP_EXEC)` to allocate from IRAM.

**Reality check:** On ESP32-S3, IRAM and DRAM share the **same physical 512KB SRAM**,
accessed via different bus addresses. IRAM grows up from 0x40370000, DRAM grows down
from 0x3FCDC700. "Free IRAM" added to heap via `MALLOC_CAP_EXEC` is the same physical
memory that would otherwise be available as DRAM heap.

**Verdict: NOT APPLICABLE.** This doesn't create new memory — it's the same RAM.
The 216KB boot heap already includes any unused IRAM regions.

### Option C: Reduce Canvas Size

| Resolution | Buffer Size | Savings vs 320×240 |
|------------|-------------|---------------------|
| 320×240 (current) | 153,600 (150KB) | — |
| 256×192 | 98,304 (96KB) | 54KB |
| 240×160 | 76,800 (75KB) | 75KB |
| 160×120 | 38,400 (37.5KB) | 113KB |

A 256×192 canvas (96KB) fits in the 139KB max block after BLE init.

**Implementation:** Render to smaller canvas, use LVGL scaling or `tft.pushImage()`
with scaling to fill the 320×240 display.

**Verdict: WORKS but visually degraded.** The game is a wireframe lunar lander —
at 256×192 it would still be playable but noticeably pixelated. At 160×120 it
becomes hard to read HUD text.

| Metric | Rating |
|--------|--------|
| Feasibility | High — 96KB fits in 139KB block |
| Memory savings | 54-113KB depending on resolution |
| Complexity | Low-Medium (scaling logic) |
| Visual/UX impact | Moderate-High (pixelation) |
| Risk | Low |

### Option D: Switch to NimBLE-Arduino

Replace the built-in ESP32 BLE Arduino (Bluedroid) with [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino).

**Documented savings:** ~100KB less RAM, ~50% less flash. Nearly 100% API-compatible
with existing code (drop-in replacement with minor changes).

**Why this helps:** The Bluedroid stack has 128KB of static sections baked into the
firmware. NimBLE's static footprint is dramatically smaller (~20-30KB). The dynamic
allocation is also smaller and may fragment less.

**Expected memory with NimBLE:**
- Boot heap: ~280-300KB (vs 216KB with Bluedroid)
- After NimBLE init: ~250-260KB free, max block ~200KB+
- Canvas 153KB: **FITS with 100KB+ to spare**

**API compatibility:** NimBLE-Arduino provides compatibility headers that map
`BLEDevice`, `BLEScan`, `BLEAdvertising`, `BLEAdvertisedDevice` to NimBLE equivalents.
The current `BlePresence.cpp` uses only:
- `BLEDevice::init()` / `BLEDevice::deinit()`
- `BLEDevice::getScan()` / `BLEDevice::getAdvertising()`
- `BLEScan` (passive scan, callbacks)
- `BLEAdvertising` (manufacturer data, service UUID)
- `BLEAdvertisedDevice` (service UUID check, manufacturer data, RSSI)

All of these are supported by NimBLE-Arduino's compatibility layer.

**Migration steps:**
1. Add `h2zero/NimBLE-Arduino` to `lib_deps` in `platformio.ini`
2. Add `-DCONFIG_BT_NIMBLE_ENABLED` to `build_flags`
3. Replace `#include <BLEDevice.h>` etc. with NimBLE equivalents (or use compat headers)
4. Minor API adjustments (e.g., `std::string` → `NimBLEAddress`, callback signatures)
5. Test advertising + scanning functionality

**Verdict: BEST OPTION.** Solves the problem at the root cause (Bluedroid's massive
static footprint) with minimal code changes. Both BLE and canvas work simultaneously.

| Metric | Rating |
|--------|--------|
| Feasibility | High — well-documented migration path |
| Memory savings | ~100KB static + ~15-20KB dynamic |
| Complexity | Low-Medium (library swap + minor API tweaks) |
| Visual/UX impact | None |
| Risk | Low — NimBLE is mature, widely used |

### Option E: Allocation Order — Canvas Before BLE

The boot log shows 180KB max contiguous block before BLE init. Canvas needs 153KB.
**If we allocate the canvas first, then init BLE, it fits.**

**How it works:**
1. At boot (or when entering game), allocate 153KB canvas → succeeds (180KB block)
2. Remaining heap: 216 - 153 = 63KB
3. Init BLE → needs ~35-40KB dynamic → succeeds (63KB available)
4. Remaining: ~23-28KB for everything else

**The catch:** The canvas is currently allocated lazily in `renderer_init()` when the
user enters the game. BLE is initialized at boot. To use this approach:
- Either pre-allocate the canvas buffer at boot and keep it forever (wastes 153KB
  when not in game)
- Or use lazy BLE: don't init BLE until after canvas is allocated, or stop BLE
  before game and restart after

**Verdict: WORKS but wasteful or complex.** Pre-allocating wastes memory when in menus.
Lazy BLE adds complexity and ~1-2s BLE startup delay.

| Metric | Rating |
|--------|--------|
| Feasibility | High — proven by boot heap numbers |
| Memory savings | 0 (reorders, doesn't save) |
| Complexity | Low (pre-alloc) or Medium (lazy BLE) |
| Visual/UX impact | None (pre-alloc) or minor delay (lazy BLE) |
| Risk | Medium — tight margins, no room for WiFi |

### Option F: Lazy BLE (Stop/Start Around Game)

Already implemented: `ble_presence_stop()` calls `BLEDevice::deinit(true)` and
`ble_presence_restart()` re-initializes. The code exists in `BlePresence.cpp`.

**Sequence:**
1. Boot → init BLE → social features work (crew, comms)
2. User enters game → `ble_presence_stop()` → BLE freed
3. Heap recovers to ~213KB free, ~180KB max block (if no other fragmentation)
4. Allocate canvas (153KB) → succeeds
5. User exits game → free canvas → `ble_presence_restart()`

**Known issue:** `BLEDevice::deinit(true)` has a documented 48-byte leak per
init/deinit cycle (ESP32 forum). After many game sessions, this accumulates.
Also, heap recovery depends on no other allocations happening between BLE init
and deinit that would prevent block coalescing.

**Verdict: VIABLE as fallback.** Works if heap defragments properly after BLE deinit.
Needs testing. The 48-byte leak is acceptable for badge use (limited sessions).

| Metric | Rating |
|--------|--------|
| Feasibility | Medium — depends on heap recovery |
| Memory savings | ~35-40KB when BLE stopped |
| Complexity | Low (stop/start calls already exist) |
| Visual/UX impact | 1-2s BLE restart delay, no social during game |
| Risk | Medium — heap recovery not guaranteed |

### Option G: Direct TFT_eSPI Rendering (No Canvas)

Eliminate the canvas entirely. Draw directly to the display controller's GRAM via SPI.

**How:** Replace `lv_canvas_*` calls with `tft.drawLine()`, `tft.drawPixel()`,
`tft.fillRect()`. The `tft` object is already globally available. The boot screen
already uses this approach.

**Saves:** 153KB (entire canvas buffer eliminated).

**Tradeoffs:**
- No double-buffering → possible tearing during frame updates
- CRT scanline post-processing effect (reads/modifies canvas buffer) must be dropped
  or reimplemented differently
- Terrain fill (currently direct buffer writes) needs column-by-column `fillRect`
- Must coordinate with LVGL (pause `lv_timer_handler` during game, or use a
  separate rendering path)

**Verdict: STRONG OPTION.** Completely eliminates the memory problem. The game is
a simple wireframe — direct rendering is fast enough. Tearing is mitigable with
draw order (clear → terrain → lines → HUD).

| Metric | Rating |
|--------|--------|
| Feasibility | High |
| Memory savings | **153KB** (entire canvas eliminated) |
| Complexity | Medium (rewrite renderer draw calls) |
| Visual/UX impact | Minor (possible tearing, no CRT effect) |
| Risk | Low |

### Option H: LVGL Draw Event (No Canvas)

Use `LV_EVENT_DRAW_MAIN` callback on a plain LVGL object. LVGL renders through its
existing partial draw buffer (7.5KB).

**Verdict: VIABLE but slower.** LVGL's draw pipeline adds per-primitive overhead.
With 40+ stars, terrain segments, lander geometry, and HUD — the overhead may
impact frame rate. Better suited for static/infrequent updates.

| Metric | Rating |
|--------|--------|
| Feasibility | Medium |
| Memory savings | **153KB** |
| Complexity | Medium |
| Visual/UX impact | Possible frame rate drop |
| Risk | Medium — performance unknown |

---

## Recommended Plan

### Phase 1: Switch to NimBLE-Arduino (PRIMARY FIX)

This is the highest-impact, lowest-risk change. It attacks the root cause: the
Bluedroid stack's 128KB static footprint.

**Steps:**
1. Add `h2zero/NimBLE-Arduino@^1.4.0` to `lib_deps`
2. Add build flags: `-DCONFIG_BT_NIMBLE_ENABLED`
3. Update `BlePresence.cpp` includes to use NimBLE compatibility headers
4. Fix any minor API differences (callback signatures, string types)
5. Build, verify heap numbers with boot log
6. Test BLE advertising + scanning + canvas allocation

**Expected result:** ~280KB boot heap, ~250KB after NimBLE init, 153KB canvas fits
with 100KB+ headroom. BLE and game run simultaneously.

**Estimated effort:** 2-4 hours.

### Phase 2: Allocation Order Guard (BELT AND SUSPENDERS)

Even with NimBLE, enforce canvas allocation before BLE init as a safety measure:

```cpp
// In renderer_init(), add heap check:
Serial.printf("[RENDERER] need=%d heap=%d max_block=%d\n",
    buf_size, ESP.getFreeHeap(), ESP.getMaxAllocHeap());
if (ESP.getMaxAllocHeap() < buf_size + 4096) {
    // Emergency: stop BLE, allocate, restart
    ble_presence_stop();
    // ... allocate canvas ...
    ble_presence_restart();
}
```

### Phase 3: Reduce LVGL Overhead (OPTIONAL)

If more headroom is needed:
- Reduce `LV_MEM_SIZE` from 64KB to 32KB (saves 32KB)
- Reduce `draw_buf` from 1/10 to 1/20 screen (saves ~4KB)
- Audit `lv_conf.h` for unused features

### Fallback: Direct TFT_eSPI Rendering

If NimBLE migration proves problematic (unlikely), fall back to Option G:
rewrite the game renderer to use direct TFT_eSPI calls. This eliminates the
canvas entirely and works regardless of BLE stack choice.

---

## Decision Matrix

| Option | Memory Fix | BLE+Game Coexist | Effort | Risk | Recommended |
|--------|-----------|------------------|--------|------|-------------|
| **D: NimBLE** | **Root cause** | **Yes** | **Low-Med** | **Low** | **✅ PRIMARY** |
| G: Direct TFT | Eliminates need | Yes | Medium | Low | ✅ Fallback |
| E: Alloc order | Workaround | Yes (tight) | Low | Med | ⚠️ Supplement |
| F: Lazy BLE | Workaround | No (alternating) | Low | Med | ⚠️ Supplement |
| C: Smaller canvas | Reduces need | Yes | Low-Med | Low | ⚠️ If needed |
| A: Static .bss | Avoids frag | Yes (barely) | Trivial | High | ❌ Too tight |
| B: IRAM | N/A | N/A | N/A | N/A | ❌ Not applicable |
| H: LVGL draw event | Eliminates need | Yes | Medium | Med | ⚠️ Alternative |

---

## Key Technical Findings

1. **IRAM/DRAM share physical RAM on ESP32-S3.** "Allocate from IRAM" doesn't create
   new memory. The boot heap already includes all available SRAM regions.

2. **Bluedroid static sections are 128KB.** This is baked into the pre-compiled Arduino
   SDK. Cannot be reduced without switching BLE stacks or building a custom SDK.

3. **NimBLE-Arduino saves ~100KB RAM** and is nearly API-compatible with the existing
   `BlePresence.cpp` code. This is the single most impactful change.

4. **The fragmentation problem is caused by BLE's dynamic allocations** splitting the
   largest contiguous heap block. Total free memory (178KB) exceeds canvas need (153KB)
   but the largest contiguous block (139KB) does not.

5. **Allocation order matters.** If canvas is allocated before BLE, the 180KB max block
   accommodates it. This is a viable workaround even without switching BLE stacks.

6. **`ble_presence_stop()` / `ble_presence_restart()` already exist** in the codebase,
   making lazy BLE a low-effort option if needed.
