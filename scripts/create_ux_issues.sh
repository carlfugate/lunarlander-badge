#!/bin/bash
# Create all UX idea issues from docs/UX_IDEAS.md
cd /Users/carlfugate/Documents/github/lunarlander-badge

# Section 1: Ambient Space Atmosphere
gh issue create --title "1.1 Starfield screensaver" --label "ux:atmosphere,priority:must-have" --body "Parallax starfield screensaver after 60s idle. White dots at 2-3 depth layers, occasional shooting stars. Touch to wake. Lightweight canvas pixel draws.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §1.1"

gh issue create --title "1.2 Earthrise screensaver" --label "ux:atmosphere,priority:should-have" --body "Slow pixel-art animation of Earth rising over lunar horizon (Apollo 8 inspired). ~40px Earth circle with rough continent shapes, grey terrain line. 90s cycle. Alternates with starfield.

**Difficulty:** Medium | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §1.2"

gh issue create --title "1.3 Orbital drift idle animation" --label "ux:atmosphere,priority:should-have" --body "Tiny lander sprite orbits a small moon in center of screen on elliptical path. Speed varies with altitude (Keplerian mechanics). Nerds will notice.

**Difficulty:** Medium | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §1.3"

gh issue create --title "1.4 Ambient LED breathing" --label "ux:atmosphere,ux:leds,priority:must-have" --body "Idle state: 6 NeoPixels do slow dim breathing pulse in deep blue (#000033→#000066). Under 15% brightness. Recognizable across a crowded hall.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★
Ref: docs/UX_IDEAS.md §1.4"

gh issue create --title "1.5 Aurora borealis LEDs" --label "ux:atmosphere,ux:leds,priority:must-have" --body "Slow color wave across 6 LEDs — greens, teals, purples blending. HSV interpolation. ~10% brightness. Beautiful in dark talk rooms.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §1.5"

gh issue create --title "1.6 Deep sleep with wake-on-touch" --label "ux:atmosphere,priority:should-have" --body "After 5min total inactivity: deep sleep (screen off, LEDs off, WiFi off). Touch interrupt wakes. Quick 'SYSTEMS REACTIVATING...' on wake. Single most important battery feature.

**Difficulty:** Medium | **Battery:** Massive savings | **Wow:** ★★
Ref: docs/UX_IDEAS.md §1.6"

# Section 2: NASA-Themed UI Polish
gh issue create --title "2.1 Mission Elapsed Time (MET) clock" --label "ux:nasa-theme,priority:must-have" --body "Status bar shows MET counter: \`MET 00:03:27:14\` counting from first boot. Always ticking, always visible. millis() math + formatted string.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §2.1"

gh issue create --title "2.2 Enhanced boot sequence with countdown" --label "ux:nasa-theme,priority:must-have" --body "Expand boot checks into full pre-launch sequence. Progress bars, buzzer tones per check. Add: Fuel cells, Life support, Guidance computer, Telemetry link. End with 'ALL SYSTEMS GO — LAUNCH COMMIT' + 3-2-1 countdown. ~4 seconds total.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §2.2"

gh issue create --title "2.3 CRT scanline overlay effect" --label "ux:nasa-theme,priority:should-have" --body "Subtle horizontal scanline overlay — every other row at 90% brightness. Retro CRT/Apollo-era feel. Togglable in settings. Implement in LVGL flush callback.

**Difficulty:** Medium | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §2.3"

gh issue create --title "2.4 Expanded comms chatter ticker" --label "ux:nasa-theme,priority:must-have" --body "Expand mission control ticker to ~50 NASA-style messages: 'CAPCOM: Go for TLI', 'FIDO: Trajectory nominal', etc. Mix real Apollo 11 transcripts + conference-specific ones. Random rotation, never the same twice.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §2.4"

gh issue create --title "2.5 Screen transition animations" --label "ux:nasa-theme,priority:must-have" --body "Horizontal wipe transitions between screens (new slides in from right). Back button reverses direction. Use LVGL built-in \`lv_scr_load_anim()\`.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★
Ref: docs/UX_IDEAS.md §2.5"

gh issue create --title "2.6 Status bar redesign — telemetry strip" --label "ux:nasa-theme,priority:should-have" --body "Redesign status bar as spacecraft telemetry: \`[▓▓▓▓░] 72% | WiFi ◉ | MET 02:14:07 | ★ 3,450\`. Segmented fuel gauge, signal icon, MET clock, best score. Monospace cyan on dark.

**Difficulty:** Medium | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §2.6"

gh issue create --title "2.7 Monospace font for numeric data" --label "ux:nasa-theme,priority:must-have" --body "Use monospace font for all numeric data (scores, MET, coordinates, fuel). Numbers don't jump when digits change — feels like an instrument panel. ~4KB flash for 14px monospace.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §2.7"

gh issue create --title "2.8 NASA color-coded alert system" --label "ux:nasa-theme,priority:must-have" --body "Standardize colors: Cyan=nominal, Green=GO/success, Amber=warning, Red=critical. Apply to boot checks, HUD, WiFi status, battery. Battery <15%=amber, <5%=red. Consistent mission control language.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★
Ref: docs/UX_IDEAS.md §2.8"

# Section 3: Social & Conference Features
gh issue create --title "3.1 Crew roster — badge discovery via ESP-NOW" --label "ux:social,priority:stretch" --body "Badges broadcast callsign via ESP-NOW/mDNS. 'Crew Roster' screen shows discovered badges sorted by first-seen. 'You've encountered 47 of 300 crew members.' People actively seek missing ones.

**Difficulty:** Hard | **Battery:** Medium | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §3.1"

gh issue create --title "3.2 Callsign selection at first boot" --label "ux:social,priority:should-have" --body "Pick a callsign from curated spacecraft names (EAGLE, COLUMBIA, ODYSSEY, etc.) or type custom (≤10 chars). Used on leaderboards, crew roster, multiplayer. Your identity for the day.

**Difficulty:** Medium | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §3.2"

gh issue create --title "3.3 Achievement trading via proximity" --label "ux:social,priority:stretch" --body "Two badges in proximity trigger 'docking sequence' animation. Exchange one random achievement each. Encourages meeting attendees. 'I need the Apollo 13 achievement — anyone have it?'

**Difficulty:** Hard | **Battery:** Medium | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §3.3"

gh issue create --title "3.4 Check-in gamification — mission log" --label "ux:social,priority:should-have" --body "Check-in = 'Launch commit'. Talk check-in = 'Orbital insertion at [talk]'. After-party = 'Lunar landing'. Each awards mission patches + XP. Mission Log screen shows conference journey as timeline.

**Difficulty:** Medium | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §3.4"

gh issue create --title "3.5 QR code badge card" --label "ux:social,priority:should-have" --body "'My Card' screen with QR code containing callsign + profile URL or vCard. Quick contact exchange without phones. Rendered locally, no network needed. Mission patch border.

**Difficulty:** Medium | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §3.5"

gh issue create --title "3.6 Distress beacon" --label "ux:social,priority:should-have" --body "Button combo broadcasts distress signal. Nearby badges see '⚠ DISTRESS BEACON: [callsign] requests assistance' with amber LED flash. Fun 'find my friend' / 'where's the CTF room?' feature.

**Difficulty:** Medium | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §3.6"

# Section 4: Gamification Beyond Lander
gh issue create --title "4.1 Achievement system — mission patches" --label "ux:gamification,priority:should-have" --body "~20 unlockable mission patches: First Landing, Eagle Scout (>5000), Apollo 13 (<1% fuel), Social Butterfly (25 badges), Marathon (10 games), Speedrunner (<30s), Perfect Landing (<0.5 vel). Patches screen shows collected vs locked.

**Difficulty:** Medium | **Battery:** None | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §4.1"

gh issue create --title "4.2 Hourly challenges" --label "ux:gamification,priority:stretch" --body "Server pushes new challenge every hour: 'Land on leftmost pad', 'Land with exactly 25% fuel', 'Slowest safe landing'. Awards bonus XP + unique hourly patch. Creates rhythm to the day.

**Difficulty:** Hard | **Battery:** Low | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §4.2"

gh issue create --title "4.3 Ghost replays" --label "ux:gamification,priority:stretch" --body "Save input sequence after each run (~200 bytes). Next run shows translucent ghost replaying previous best. Could download #1 player's ghost from server. Racing your ghost is addictive.

**Difficulty:** Hard | **Battery:** None | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §4.3"

gh issue create --title "4.4 Mission Control jumbotron leaderboard" --label "ux:gamification,ux:conference,priority:stretch" --body "Live leaderboard on projector display. Top 10 with callsigns, scores, landing replays. New #1 triggers celebration + player's badge LEDs go gold. Seeing your callsign on a giant screen is unforgettable.

**Difficulty:** Medium (badge) / Hard (jumbotron) | **Battery:** None | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §4.4"

gh issue create --title "4.5 Difficulty progression — unlock gates" --label "ux:gamification,priority:must-have" --body "Lock Medium until landing on Easy. Lock Hard until landing on Medium. Unlock triggers 'FLIGHT DIRECTOR: Crew cleared for [difficulty] operations' + LED celebration. Natural progression arc.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §4.5"

gh issue create --title "4.6 Lander cosmetic skins" --label "ux:gamification,priority:should-have" --body "Unlock alternate lander skins via achievements: gold (#1 score), red (10 crashes), rainbow (50 badges discovered), skull (CTF progress). Palette swaps — minimal memory cost.

**Difficulty:** Medium | **Battery:** None | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §4.6"

gh issue create --title "4.7 Speed run timer mode" --label "ux:gamification,priority:must-have" --body "Dedicated speed run mode with precise millisecond timer. Measures first thrust to successful landing. Separate leaderboard category. Speed runners generate hype all day.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §4.7"

gh issue create --title "4.8 Tournament bracket mode" --label "ux:gamification,priority:stretch" --body "Server runs 16-player single elimination, best-of-3. Brackets on jumbotron. Badge push notification: 'FLIGHT: [callsign], report to launch pad. Match in T-minus 2:00'. Winner gets unique achievement.

**Difficulty:** Hard | **Battery:** Low | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §4.8"

# Section 5: LED Integration
gh issue create --title "5.1 Thrust glow LEDs during gameplay" --label "ux:leds,priority:must-have" --body "Bottom 2 LEDs glow orange-to-white proportional to thrust. Flare on touch, fade on release (~200ms). Visceral connection between input and physical badge. Visible to people watching over your shoulder.

**Difficulty:** Easy | **Battery:** Medium | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §5.1"

gh issue create --title "5.2 Landing celebration LEDs" --label "ux:leds,priority:must-have" --body "Successful landing: 6 LEDs flash green 3x, settle into slow green pulse 5s. Crash: rapid red flash, fade to black. New high score: gold sparkle (random white/gold flashes). LEDs as emotional response.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §5.2"

gh issue create --title "5.3 Fuel gauge LEDs during gameplay" --label "ux:leds,priority:must-have" --body "6 LEDs as fuel gauge. Full=all green. Depletes from edges inward. <20%: remaining turn amber. <10%: red + blinking. Critical game info in peripheral vision.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §5.3"

gh issue create --title "5.4 Multiplayer team color LEDs" --label "ux:leds,priority:must-have" --body "In multiplayer, each player assigned a team color. Badge LEDs glow that color at low brightness throughout match. Visible across conference. Instant tribal identity.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §5.4"

gh issue create --title "5.5 Proximity pulse LEDs" --label "ux:leds,ux:social,priority:should-have" --body "Two badges detect each other via ESP-NOW → both pulse cyan briefly. New discovery = brighter/longer pulse. You'll notice pulses in crowded hallways: 'someone new is nearby.'

**Difficulty:** Medium (tied to crew roster) | **Battery:** Low | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §5.5"

gh issue create --title "5.6 Achievement unlock LED fanfare" --label "ux:leds,ux:gamification,priority:must-have" --body "Achievement unlock: rainbow sweep left-to-right, ending on gold. 2 seconds. Pavlovian — people chase the rainbow sweep. Visible to nearby people who ask 'what did you unlock?'

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §5.6"

gh issue create --title "5.7 Heartbeat status LED indicator" --label "ux:leds,priority:must-have" --body "Main menu: 1 LED does slow double-pulse (heartbeat) in dim cyan = alive + connected. WiFi drops = amber single-pulse. Battery critical = dim red. 1 LED, always communicating status.

**Difficulty:** Easy | **Battery:** Very Low | **Wow:** ★★
Ref: docs/UX_IDEAS.md §5.7"

# Section 6: Audio Design
gh issue create --title "6.1 Boot sequence tones" --label "ux:audio,priority:must-have" --body "Each boot check gets rising tone: 440→554→659→880Hz (A-major arpeggio), 80ms each. Final 'LAUNCH COMMIT': 880→1760Hz octave jump. Total ~1 second. Sets the tone for the experience.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §6.1"

gh issue create --title "6.2 Menu navigation click sounds" --label "ux:audio,priority:must-have" --body "Soft 50ms clicks at 2000Hz on select. 1500Hz on back. Higher=forward, lower=backward. Keep quiet and short — nobody wants beeping during talks.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★
Ref: docs/UX_IDEAS.md §6.2"

gh issue create --title "6.3 Enhanced thrust audio" --label "ux:audio,priority:must-have" --body "Continuous low tone (150Hz) while thrusting, rises in pitch with duration (engine strain). Release: descending chirp 200→80Hz, 100ms. Makes the lander feel physical. Pair with thrust LED glow.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §6.3"

gh issue create --title "6.4 Altitude warning — radar altimeter beeps" --label "ux:audio,priority:must-have" --body "Below 50m: repeating 1000Hz beep. 1/sec at 50m, 2/sec at 25m, 4/sec at 10m, continuous at 5m. Exactly like a radar altimeter. Pilots will recognize it. Everyone else feels the tension.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §6.4"

gh issue create --title "6.5 Landing and crash jingles" --label "ux:audio,priority:must-have" --body "Landing: 5-note ascending melody (C5-E5-G5-C6-E6, 80ms each). Crash: descending chromatic slide 800→100Hz, 300ms. New high score: landing jingle + 3-note fanfare. Emotional punctuation.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §6.5"

gh issue create --title "6.6 Achievement unlock sound" --label "ux:audio,ux:gamification,priority:must-have" --body "Distinctive 4-note jingle on achievement unlock: two quick ascending notes, pause, high sustained note (Zelda 'item get' in buzzer form). Recognizable from across the room after hearing once.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §6.6"

gh issue create --title "6.7 Countdown timer tones" --label "ux:audio,priority:must-have" --body "Tournament/challenge countdown: 1000Hz beep/sec from T-10 to T-3, three rapid 1500Hz at T-3/2/1, long 2000Hz at T-0. Classic rocket launch cadence.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §6.7"

gh issue create --title "6.8 Silent mode toggle" --label "ux:audio,priority:must-have" --body "Global mute via system menu or hold Back 2s. 🔇 icon in status bar when muted. Global flag checked before every tone() call. Respects the conference environment.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★ (essential)
Ref: docs/UX_IDEAS.md §6.8"

# Section 7: Easter Eggs & Delight
gh issue create --title "7.1 Konami code easter egg" --label "ux:easter-egg,priority:must-have" --body "Main menu: tap edges in Konami pattern (↑↑↓↓←→←→). Unlocks 'God Mode' bling (all LEDs full rainbow) + 'CHEAT ACTIVATED' screen + secret pirate ship lander skin (BadgePirates nod).

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §7.1"

gh issue create --title "7.2 Apollo 11 transcript replay" --label "ux:easter-egg,priority:must-have" --body "Hidden in credits (tap BadgePirates logo 5x): ticker replays actual Apollo 11 landing transcript in real-time pacing. '60 seconds...' → 'Tranquility Base here. The Eagle has landed.' ~3 minutes. Goosebumps.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §7.2"

gh issue create --title "7.3 1202 Alarm error handler" --label "ux:easter-egg,priority:must-have" --body "Real software errors display '1202 ALARM' in amber (same alarm that nearly aborted Apollo 11). Actual error details below in smaller text. Turns bugs into features.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §7.3"

gh issue create --title "7.4 Hidden game: Asteroids" --label "ux:easter-egg,priority:stretch" --body "Tap 'System Info' 7x rapidly: simple Asteroids clone. Same touch zones as Lander. Circles that split when shot. No scoring/saves — just a hidden toy. The fact it exists is the point.

**Difficulty:** Hard | **Battery:** None | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §7.4"

gh issue create --title "7.5 Famous space quotes on boot" --label "ux:easter-egg,priority:must-have" --body "Random space quote on each boot splash: Armstrong, Swigert, Tsiolkovsky, Aldrin, Kranz, 'Ad astra per aspera' (full Kansas motto). ~20 quotes. Costs nothing, adds personality.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §7.5"

gh issue create --title "7.6 Secret bling: Morse code 'AD ASTRA'" --label "ux:easter-egg,ux:leds,priority:should-have" --body "Hidden LED mode unlocked at 10+ achievements: 6 LEDs encode 'AD ASTRA' in Morse code via cyan flashes. Only Morse-literate people decode it. A secret within a secret.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §7.6"

gh issue create --title "7.7 Developer credits crawl" --label "ux:easter-egg,priority:should-have" --body "After credits scroll, wait 10s: Star Wars-style text crawl telling badge dev story. 'In a city on the plains... badges were lost to customs... but the crew would not be stopped...' Ends with tiny lander landing.

**Difficulty:** Medium | **Battery:** None | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §7.7"

gh issue create --title "7.8 The Answer — 42 easter egg" --label "ux:easter-egg,priority:must-have" --body "When MET clock hits exactly 04:02:00 (4h 2m = '42'): all LEDs flash white once, ticker shows 'DEEP THOUGHT: The answer is 42.' Blink and you miss it.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §7.8"

# Section 8: Conference Integration
gh issue create --title "8.1 Talk reminders with countdown" --label "ux:conference,priority:should-have" --body "Schedule viewer gets 'Set Reminder' button. T-minus 5: buzz + cyan LEDs + ticker 'FLIGHT: [Talk] launching in T-minus 5:00 — [Room]'. T-minus 1: another buzz. NASA countdown cadence.

**Difficulty:** Medium | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §8.1"

gh issue create --title "8.2 Live CTF scoreboard" --label "ux:conference,priority:should-have" --body "Dedicated screen polling CTF API every 60s. Top 10 teams with scores. Your team highlighted green. Score changes trigger LED flash. Competitive energy visible all day.

**Difficulty:** Medium | **Battery:** Medium | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §8.2"

gh issue create --title "8.3 Hallway track beacon — crowd finder" --label "ux:conference,priority:stretch" --body "Badges form ad-hoc clusters via ESP-NOW. Screen shows: 'Lobby: 23 crew | Track A: 45 crew | CTF Room: 12 crew'. Find where people are congregating. Organizers push location labels.

**Difficulty:** Hard | **Battery:** Medium | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §8.3"

gh issue create --title "8.4 After-party countdown" --label "ux:conference,priority:must-have" --body "From 5PM: status bar shows 'PARTY T-minus 02:34:17'. Ticker escalates: 'Post-mission debrief approaching' → 'Prepare for splashdown' → '🎉 SPLASHDOWN'. At T-0: rainbow LEDs full brightness.

**Difficulty:** Easy | **Battery:** Low | **Wow:** ★★★★
Ref: docs/UX_IDEAS.md §8.4"

gh issue create --title "8.5 OTA update station UI polish" --label "ux:conference,priority:must-have" --body "Polish OTA screen as NASA-style: 'UPLOADING FLIGHT SOFTWARE... Block 147/512 [▓▓▓▓▓▓░░░░] 28%'. Enables pushing bug fixes, new challenges, surprise features throughout the day.

**Difficulty:** Easy | **Battery:** Medium | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §8.5"

gh issue create --title "8.6 Speaker rating — mission debrief" --label "ux:conference,priority:should-have" --body "After talk end time (if reminder set): prompt 'MISSION DEBRIEF: Rate [Talk]'. Three buttons: 🚀 great, 🛰️ good, 💥 rough landing. One tap, anonymous, sent to server. Space metaphors keep it fun.

**Difficulty:** Medium | **Battery:** Low | **Wow:** ★★★
Ref: docs/UX_IDEAS.md §8.6"

gh issue create --title "8.7 Conference WiFi auto-connect" --label "ux:conference,priority:must-have" --body "Auto-connect to conference WiFi on boot (SSID/password in firmware). No manual setup for 90% of attendees. Boot shows 'Comms link [OK] — BSidesKC-2026'. Removes biggest friction point.

**Difficulty:** Easy | **Battery:** None | **Wow:** ★★
Ref: docs/UX_IDEAS.md §8.7"

gh issue create --title "8.8 Mission Control jumbotron feed" --label "ux:conference,priority:stretch" --body "Server pushes live feed to venue projector: leaderboard, live spectating, tournament brackets, hourly challenges, total badges online, total landings/crashes, notable events ticker. The heartbeat of the badge experience.

**Difficulty:** Hard | **Battery:** None | **Wow:** ★★★★★
Ref: docs/UX_IDEAS.md §8.8"

echo "Done creating all issues!"
