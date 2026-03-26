# LinkedIn Post Drafts: Building the Test Harness

Three angles on the testing/QA story. Pick one or combine elements.

---

## Draft 1: "The AI wrote the code. Then I made it prove it worked." (Long form)

Building firmware with AI is the easy part. The hard part is trusting it.

I used AI agents to build an ESP32 conference badge — Lunar Lander game, BLE social features, animated screensavers, the works. It compiled. It ran. Then it crashed. Not immediately — after 7 minutes of bling animations. Then again when you opened the game after using the LED patterns. Then a null pointer dereference when navigating back from any menu.

AI-generated code has the same bugs as human code. LVGL widget lifecycle issues. FreeRTOS task safety violations. Memory leaks that only show up under sustained load. The difference is you can't ask the AI "does this work?" — it'll say yes every time.

So I built a test harness. A serial command protocol (BSTP v1.0) baked into the firmware behind a feature flag. Every screen, every function, every LED pattern is callable over USB serial. A Python script sends commands, monitors responses, detects crashes, and self-recovers when the badge reboots.

The overnight soak test runs for 8 hours. It navigates every screen, cycles every bling mode, toggles audio, changes callsigns, sends BLE messages — all in random combinations designed to trigger the concurrent operation bugs that sequential testing misses. It tracks heap every cycle, flags leaks over 5KB, and logs everything to CSV for trend analysis.

The bugs it found were real: a Ticker ISR calling NeoPixel functions through a FreeRTOS queue (cross-task assert), LVGL screens being double-freed during animated transitions, timer callbacks writing to deleted widgets. None of these showed up in the 47 unit tests. All of them showed up in production.

The test harness is 200 lines of C++ on the badge, 300 lines of Python on the host. It's designed to be backportable to any ESP32/LVGL project. The protocol is documented. The scripts are open source.

AI can write your code. But someone still has to build the system that proves the code works. That's the engineering.

github.com/carlfugate/lunarlander-badge

#EmbeddedSystems #QualityEngineering #AI #Testing #FirmwareDevelopment #BSidesKC #OpenSource

---

## Draft 2: Short and punchy

The AI wrote 15,000 lines of C++ for a conference badge. It compiled on the first try.

Then it crashed after 7 minutes.

The fix wasn't better prompts. It was a serial test harness — every screen, every function, every LED pattern callable over USB. An overnight soak test that runs random combinations for 8 hours, detects crashes, self-recovers, and tracks memory leaks.

The bugs it found: FreeRTOS task safety violations, LVGL double-frees, timer callbacks hitting deleted widgets. None appeared in unit tests. All appeared on hardware.

AI doesn't eliminate QA. It makes QA the bottleneck. Build accordingly.

#Testing #AI #EmbeddedSystems #QualityEngineering

---

## Draft 3: Technical leadership angle

"Ship it" means something different when the code runs on hardware someone wears around their neck for 12 hours.

I directed AI agents to build firmware for a BSidesKC conference badge. The code was impressive — game engine, BLE social features, animated screensavers. But "it compiles" isn't "it works." And "it works on my desk" isn't "it works when 300 people are mashing buttons all day."

So before I trusted the firmware, I built the system to break it:

A serial command protocol where every function is testable over USB. A soak test that runs concurrent operations — bling animations while navigating screens while BLE is scanning — because that's what real users do. Crash detection that reads the panic output, reconnects, and keeps testing. Heap tracking that catches the slow leaks unit tests never find.

The pattern: AI writes code fast. Hardware reveals truth slowly. The gap between those two speeds is where engineering lives.

The test harness is open source and backportable to any ESP32/LVGL badge project. Because the next person who ships AI-generated firmware to 300 conference attendees should have better tools than "flash and pray."

#QualityEngineering #AI #EmbeddedSystems #Testing #BSidesKC
