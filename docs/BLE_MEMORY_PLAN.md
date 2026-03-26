# BLE Memory Plan — Display Buffer Research

## Current Memory Layout

| Buffer | Size | Source |
|--------|------|--------|
| LVGL canvas (game) | 153,600 B (150 KB) | `320×240×2` RGB565 in `LunarRenderer.cpp` |
| LVGL draw_buf | 7,680 B (7.5 KB) | `240×320/10` in `main.cpp` |
| LVGL internal heap | 65,536 B (64 KB) | `LV_MEM_SIZE` in `lv_conf.h` |
| **Total display-related** | **~221.5 KB** | |

ESP32-S3 (no PSRAM) has ~280–320 KB free heap after boot.
NimBLE needs ~40–60 KB. The 150 KB canvas is the main obstacle.

## The Core Problem

The game renderer (`LunarRenderer.cpp`) allocates a full 320×240 RGB565 canvas buffer
(150 KB) via `heap_caps_malloc`. It tries PSRAM first, falls back to internal SRAM.
Without PSRAM, this single allocation consumes ~50% of available heap, leaving
insufficient room for BLE.

---

## Option 1: Replace LVGL Canvas with Direct TFT_eSPI Rendering (RECOMMENDED)

**Saves: ~150 KB** (eliminates canvas buffer entirely)

The boot screen already uses raw TFT_eSPI (`tft.fillScreen`, `tft.setCursor`,
`tft.println`) — the `tft` object is globally accessible via `extern TFT_eSPI tft`
in `Screen_Module.h`.

**How it works:**
- Instead of drawing to an in-memory canvas buffer, draw directly to the display
  controller's internal GRAM via SPI using TFT_eSPI primitives:
  - `tft.fillScreen(TFT_BLACK)` — clear frame
  - `tft.drawPixel(x, y, color)` — stars, lander center dot
  - `tft.drawLine(x1, y1, x2, y2, color)` — terrain, lander wireframe, thrust flame
  - `tft.fillRect(x, y, w, h, color)` — terrain fill (column-by-column)
  - `tft.setAddrWindow()` + `tft.pushColors()` — bulk pixel writes for CRT scanline effect
- HUD elements (fuel, speed, alt, timer) can use `tft.setCursor` + `tft.print`
  or remain as LVGL labels overlaid on a separate LVGL screen

**Tradeoffs:**
- Drawing directly to display means no double-buffering → possible tearing/flicker
  during frame updates (mitigated by fast SPI clock and drawing order)
- Must pause LVGL rendering during game (call `lv_display_set_buffers` with NULL
  or simply don't call `lv_task_handler` during game loop)
- Terrain fill via `drawPixel` per-column is slower than memset on a buffer, but
  `tft.fillRect` per-column or `pushColors` per-scanline can compensate
- The CRT scanline post-process (current code reads back from canvas_buf) would
  need to be dropped or done differently (e.g., draw every other row slightly darker
  during initial draw)

**Complexity:** Medium — requires rewriting `renderer_draw()` to use TFT_eSPI calls
instead of `lv_canvas_set_px` / `draw_line` / direct buffer manipulation. The
drawing logic (world-to-screen transforms, lander geometry) stays identical.

---

## Option 2: TFT_eSPI Sprite as Smaller Strip Buffer

**Saves: ~100–140 KB** (depending on strip height)

Use `TFT_eSprite` with a narrow horizontal strip (e.g., 320×16 = 10 KB) and render
the scene in strips, pushing each to the display:

```cpp
TFT_eSprite strip = TFT_eSprite(&tft);
strip.createSprite(320, 16);  // 10,240 bytes
for (int y = 0; y < 240; y += 16) {
    // render all game elements that intersect rows y..y+15 into strip
    strip.pushSprite(0, y);
}
```

**Tradeoffs:**
- Every game element (terrain, stars, lander, lines) must be clipped/rendered
  per-strip — significantly more complex than Option 1
- 15 SPI transactions per frame instead of 1 continuous draw
- Still needs a buffer, just smaller (10 KB vs 150 KB)
- Harder to maintain and debug

**Complexity:** High — every draw call needs strip-aware clipping.

---

## Option 3: LVGL Draw Event Callback (No Canvas)

**Saves: ~150 KB** (no canvas buffer)

LVGL v9 supports `LV_EVENT_DRAW_MAIN` on any `lv_obj`. Create a plain object
covering the screen and draw game elements in the callback using `lv_draw_line`,
`lv_draw_rect`, etc. LVGL renders through its own partial buffer (the existing
7.5 KB draw_buf):

```cpp
lv_obj_t *game_obj = lv_obj_create(parent);
lv_obj_set_size(game_obj, 320, 240);
lv_obj_add_event_cb(game_obj, game_draw_cb, LV_EVENT_DRAW_MAIN, &game_state);

static void game_draw_cb(lv_event_t *e) {
    lv_layer_t *layer = lv_event_get_layer(e);
    // Use lv_draw_line, lv_draw_rect, etc. on layer
}
```

**Tradeoffs:**
- No dedicated framebuffer needed — LVGL renders through its partial draw_buf
- LVGL's draw pipeline adds overhead per primitive (not ideal for 60+ draw calls/frame)
- Terrain fill (currently done via direct buffer writes) would need `lv_draw_rect`
  per column segment — potentially slow
- CRT scanline effect impossible without a full buffer to post-process
- HUD labels work naturally as sibling LVGL objects
- LVGL controls when/how often the draw callback fires — less control over frame timing

**Complexity:** Medium — rewrite draw calls to use `lv_draw_*` API instead of
canvas API. Similar effort to Option 1 but stays within LVGL ecosystem.

---

## Option 4: Reduce LVGL Buffers (Incremental Savings)

These can be combined with any option above:

| Tweak | Current | Proposed | Savings |
|-------|---------|----------|---------|
| `LV_MEM_SIZE` | 64 KB | 32 KB | 32 KB |
| `draw_buf` | 1/10 screen (7.5 KB) | 1/20 screen (3.8 KB) | 3.7 KB |
| Disable unused LVGL features | — | Audit `lv_conf.h` | Variable |

**Notes on LV_MEM_SIZE reduction:**
- LVGL v9 uses more memory than v8 (~5–6 KB more baseline)
- 32 KB works for simple UIs (labels, bars, buttons) — the menu screens
- The game screen has ~8 LVGL objects (canvas + 5 labels + 1 bar + 1 warn label)
- If using Option 1 (direct TFT), game screen needs 0 LVGL objects → 32 KB is safe
- Monitor with `lv_mem_monitor()` to find actual peak usage

**Notes on draw_buf reduction:**
- LVGL docs say 1/10 is "recommended" but not minimum
- Community reports 5 rows (`320×5×2 = 3,200 B`) working fine on 320×240
- Smaller buffer = more flush cycles = slightly slower UI updates
- For this project (simple menu UI), the difference is negligible

---

## Option 5: Hybrid — LVGL for Menus, Direct TFT for Game

**This is the practical recommendation.** Combine Options 1 + 4:

1. Keep LVGL for menu/settings screens (it's already working)
2. When entering game: call `renderer_cleanup()`, then draw game using raw TFT_eSPI
3. When exiting game: re-initialize LVGL screen
4. Reduce `LV_MEM_SIZE` to 32 KB
5. Reduce `draw_buf` to 1/20 screen

**Memory during game (with BLE active):**
- No canvas buffer: 0 KB (was 150 KB)
- LVGL idle but resident: ~32 KB heap + 3.8 KB draw_buf
- BLE (NimBLE): ~50 KB
- Remaining free: ~180+ KB for game logic, stack, etc.

**Memory during menus (with BLE active):**
- LVGL active: ~32 KB heap + 3.8 KB draw_buf
- BLE (NimBLE): ~50 KB
- No canvas: 0 KB
- Remaining free: ~200+ KB

---

## Recommendation Summary

| Priority | Action | Savings | Effort |
|----------|--------|---------|--------|
| 1 | Replace canvas with direct TFT_eSPI in game renderer | **150 KB** | Medium |
| 2 | Reduce `LV_MEM_SIZE` 64→32 KB | **32 KB** | Trivial |
| 3 | Reduce `draw_buf` to 1/20 screen | **~4 KB** | Trivial |
| — | **Total freed** | **~186 KB** | |

This frees enough heap for NimBLE BLE (~50 KB) with comfortable margin.

The key insight: the `tft` object already exists globally, the boot screen already
proves direct TFT_eSPI rendering works on this hardware, and the game's drawing
logic (lines, pixels, fills) maps 1:1 to TFT_eSPI primitives. The canvas was
convenient but is the single largest memory consumer.
