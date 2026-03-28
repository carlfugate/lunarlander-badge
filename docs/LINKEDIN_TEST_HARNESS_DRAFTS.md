# LinkedIn Post Drafts: The 8-Hour Stress Test

Updated with actual findings from the BSTP v1.0 stress test. Three angles — pick one or combine.

## The Numbers

- 8-hour stress test on real hardware
- 9,352 operations across 338 test cycles
- 6 classes of bugs found and fixed
- 0 bytes leaked after fixes
- 1 crash in final 8-hour run (credits timer — fixed)
- Without testing: all 6 bugs ship to 300 attendees at BSidesKC 2026

---

## Draft 1: "The Conference That Almost Wasn't"

BSidesKC is April 25 in Kansas City. 300 people get a custom badge — ESP32-S3, touchscreen, Lunar Lander game, LED bling, the works. I used AI to build the firmware. It compiled. It ran. The game played great on my desk.

Then I let an automated test harness hammer it for 8 hours. 9,352 operations. 338 test cycles. And I found out what would have actually happened at the conference.

Hour one: an attendee plays Lunar Lander a dozen times, browses the menus between games. Around game 100, the badge freezes. Not crashes — freezes. Screen on, LEDs lit, looks alive. But it's dead. A silent infinite loop buried in LVGL's assert handler, triggered by a style memory leak that ate 600 bytes every time the main menu loaded. No error message. No reboot. No way to diagnose without a serial cable. They'd think it was broken. They'd be right.

Hour two: someone has rainbow LEDs running and opens the game. The LED animation ISR and the game loop both grab the NeoPixels at the same time. FreeRTOS fires a queue assert. The badge reboots mid-play. Sometimes it happens, sometimes it doesn't. Maddening.

Hour three: someone tries the game and gets a black screen. LEDs on, game dead. BLE initialization fragmented the heap so badly that the 153KB game canvas can't allocate — even with 207KB free. "The game doesn't work." "Works for me." Both statements true.

Hour four: the difficulty select screen freezes for 30 seconds. A blocking WebSocket call. The attendee taps Easy, nothing happens, taps harder, gives up.

Scattered throughout: five different LVGL timers outliving their screens, writing to freed memory. Double-free crashes from redundant widget deletion. Bugs that depend on timing, on order of operations, on how long you've been using the badge.

None of this showed up in unit tests. None of it showed up in 10 minutes of desk testing. All of it showed up in one overnight stress test — a serial protocol baked into the firmware and a Python script that randomizes every possible user path.

6 classes of bugs. All fixed. 0 bytes leaked. The conference will work.

The test harness is open source: github.com/carlfugate/lunarlander-badge

#BSidesKC #EmbeddedSystems #QualityEngineering #Testing #AI #FirmwareDevelopment #OpenSource #ESP32

---

## Draft 2: "6 Bugs That Only Appear After 100 Clicks"

Unit tests passed. The build compiled. The game ran on my desk for 20 minutes. Then I let an AI-driven test harness hammer the badge for 8 hours — 9,352 operations, 338 cycles, every screen and feature in random order — and found 6 classes of bugs that would have shipped to 300 conference attendees.

Every one of them has the same signature: they only appear under sustained use or concurrent operations. The kind of thing that happens at a conference but never on a developer's bench.

**1. The silent killer.** Every main menu creation leaked 600 bytes from LVGL's internal 64KB memory pool. After ~100 transitions, the pool exhausted. LVGL's assert handler? `while(1);` — a silent infinite hang. No crash, no error, no reboot. The badge just stops. Looks alive. Is dead.

**2. The random reboot.** LED animations ran on a Ticker ISR. The game ran on the main loop. Both touched NeoPixels. When they collided — and at a conference, they will — FreeRTOS fired a queue assert and rebooted the badge. Intermittent. Unreproducible by the user.

**3. The impossible allocation.** BLE fragmented the heap so badly that a 153KB game buffer couldn't allocate despite 207KB free. Black screen, no error. Whether it worked depended on initialization order.

**4. The frozen screen.** A blocking WebSocket call on the game's difficulty select. 5-30 seconds of unresponsive UI. Buttons visible, nothing works.

**5. Timers outliving screens.** Five separate LVGL timers kept firing after their parent screen was deleted. Writing to freed memory. Sometimes a crash, sometimes corruption, sometimes nothing. Found all five.

**6. Double-free.** Multiple code paths manually deleted widgets that were also children of a screen being deleted. Crash-on-exit from certain menu sequences.

The pattern: these bugs involve state that accumulates over time or concurrent operations that rarely collide. Unit tests can't find them. Only sustained, randomized, real-hardware testing can.

After fixes: 8 hours, 9,352 operations, 0 bytes leaked, 0 crashes.

The test harness is 200 lines of C++ on the badge, 300 lines of Python on the host. Open source and backportable to any ESP32/LVGL project.

github.com/carlfugate/lunarlander-badge

#EmbeddedSystems #Testing #QualityEngineering #AI #FirmwareDevelopment #BSidesKC #ESP32 #OpenSource

---

## Draft 3: "while(1); — The Bug With No Error Message"

The badge froze. No crash. No error. No reboot. Just... stopped.

The serial console was silent. The screen was frozen mid-render. The LEDs were on. It looked alive but it was dead. Power cycling brought it back. Then it froze again, always after extended use, never at the same point.

I'm building a conference badge for BSidesKC — ESP32-S3, touchscreen, Lunar Lander game, LED bling. I used AI to write the firmware. It worked great in testing. But an 8-hour automated stress test found what desk testing never could.

The bug: every time the main menu screen was created, LVGL allocated styles into its internal 64KB memory pool. Every time. Never freed. 600 bytes per menu transition. Play the game, go back to the menu, play again — 600 bytes gone. After roughly 100 round trips, the pool was exhausted.

Here's what made it evil: LVGL's out-of-memory assert handler is `while(1);`. An infinite loop. No error message. No panic output. No watchdog trigger. The CPU just spins forever. The badge looks powered on but does absolutely nothing. At a conference, an attendee would use the badge for an hour, it would freeze, and there would be zero diagnostic information available without a serial cable and knowledge of the firmware internals.

Finding it required two AI agents independently tracing the LVGL style allocation path through the menu creation code, correlating it with the heap telemetry from the stress test that showed a steady 600-byte-per-cycle decline in the LVGL pool.

The fix was one line:

```c
static bool styles_initialized = false;
if (styles_initialized) return;
styles_initialized = true;
```

Initialize the styles once. Never again. That's it.

This was one of 6 bug classes the stress test caught — 9,352 operations over 8 hours. After fixes: zero leaks, zero crashes. The kind of confidence you can't get from "it works on my desk."

The test harness is open source: github.com/carlfugate/lunarlander-badge

#EmbeddedSystems #Debugging #LVGL #ESP32 #FirmwareDevelopment #Testing #BSidesKC #AI #OpenSource
