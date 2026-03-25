# UX Design Ideas — BSidesKC 2026 Badge

> **Hardware:** ESP32-S3 · 320×240 touchscreen · 6 NeoPixels · Buzzer · WiFi · SD card · Battery powered
> **Theme:** *Ad Astra* — "To the Stars" (Kansas state motto) · NASA · Lunar Lander · Mission Control
> **Conference:** BSidesKC · April 25, 2026 · Kansas City
> **Badge Design:** [BadgePirates / BPLabs](https://bplabs.tech)
>
> Last year the badges got stuck in customs and barely got used. This year we make the firmware so good people can't put them down.

---

## 1. Ambient Space Atmosphere

The badge should feel like a living piece of space hardware even when nobody's actively using it. These idle states reinforce the theme, reward glances, and make the badge feel worth wearing all day.

### 1.1 Starfield Screensaver

After 60 seconds of inactivity, the screen fades to a classic parallax starfield — white dots drifting at 2–3 depth layers, with occasional slow-moving shooting stars. Touch anywhere to wake. Stars are just `lv_canvas_set_px()` calls on a black background — extremely lightweight.

- **Difficulty:** Easy
- **Battery Impact:** Low (static pixels, no LED, slow refresh)
- **Wow Factor:** ★★★

### 1.2 Earthrise Screensaver

A slow pixel-art animation of Earth rising over the lunar horizon, inspired by the famous Apollo 8 photo. The Earth is a small ~40px circle with rough continent shapes, rising over a grey terrain line. Cycles every ~90 seconds. Could alternate randomly with the starfield.

- **Difficulty:** Medium (sprite art + smooth animation)
- **Battery Impact:** Low
- **Wow Factor:** ★★★★

### 1.3 Orbital Drift

A tiny lander sprite orbits a small moon in the center of the screen, following a simple elliptical path. Orbital speed varies with altitude (faster at perigee, slower at apogee) — real Keplerian mechanics in miniature. Nerds will notice. Everyone else just sees a cool animation.

- **Difficulty:** Medium (math is simple but tuning the feel takes time)
- **Battery Impact:** Low
- **Wow Factor:** ★★★★

### 1.4 Ambient LED Breathing

When idle, the 6 NeoPixels do a slow, dim "breathing" pulse in deep blue (#000033 → #000066) — like the glow of instrument panels in a dark cockpit. Brightness stays under 15% to save battery. Instantly recognizable across a crowded conference hall as "that's a BSidesKC badge."

- **Difficulty:** Easy
- **Battery Impact:** Low (very dim LEDs)
- **Wow Factor:** ★★

### 1.5 Aurora Borealis LEDs

A slow color wave rolls across the 6 LEDs — greens, teals, and purples blending into each other, mimicking aurora effects seen from the ISS. Runs at ~10% brightness. Beautiful in a dark talk room.

- **Difficulty:** Easy (HSV interpolation across 6 pixels)
- **Battery Impact:** Low
- **Wow Factor:** ★★★

### 1.6 Deep Sleep with Wake-on-Touch

After 5 minutes of total inactivity (including screensaver), the badge enters deep sleep — screen off, LEDs off, WiFi off. Touch interrupt wakes it. A quick "SYSTEMS REACTIVATING..." boot message plays on wake (faster than full boot). This is the single most important battery feature.

- **Difficulty:** Medium (ESP32 deep sleep + touch wake IRQ + state restore)
- **Battery Impact:** Massive savings (this is the battery feature)
- **Wow Factor:** ★★ (invisible when it works, which is the point)


---

## 2. NASA-Themed UI Polish

Every screen should feel like you're operating a piece of space hardware. The goal is a cohesive mission control aesthetic — not a phone app that happens to be on a badge.

### 2.1 Mission Elapsed Time (MET) Clock

Replace the status bar clock with a Mission Elapsed Time counter: `MET 00:03:27:14` counting up from when the badge first booted at the conference. This is how NASA tracks time during missions. It's always ticking, always visible, always reinforcing that you're on a mission. Resets on power cycle (or persists to SD if we're fancy).

- **Difficulty:** Easy (millis() math + formatted string)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 2.2 Boot Sequence Enhancement

Expand the existing boot checks into a full "pre-launch sequence." Each check gets a brief progress bar that fills, a green `[OK]` or amber `[WARN]`, and a short buzzer tone. Add checks for: `Fuel cells`, `Life support`, `Guidance computer`, `Telemetry link`. End with `ALL SYSTEMS GO — LAUNCH COMMIT` in bright green, then a 3-2-1 countdown with buzzer tones before the main menu appears. The whole thing should take ~4 seconds — long enough to feel cinematic, short enough to not annoy.

- **Difficulty:** Easy (mostly string sequencing + timing)
- **Battery Impact:** None (one-time at boot)
- **Wow Factor:** ★★★★

### 2.3 CRT Scanline Effect

A subtle horizontal scanline overlay on all screens — every other row is drawn at 90% brightness instead of 100%. This gives the entire UI a retro CRT / Apollo-era monitor feel. Togglable in settings for people who hate it. Implementation: just darken even-numbered rows in the LVGL flush callback.

- **Difficulty:** Medium (LVGL flush callback modification)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 2.4 Comms Chatter Ticker

Expand the existing mission control ticker at the bottom of the main menu with a library of ~50 NASA-style messages that rotate randomly: `CAPCOM: Go for TLI`, `FIDO: Trajectory nominal`, `EECOM: Cryo pressure stable`, `FLIGHT: We are GO`, `RETRO: Entry corridor confirmed`. Mix in real Apollo 11 transcripts. Add conference-specific ones: `CAPCOM: Next talk in Bay A, T-minus 10`, `FIDO: Coffee supply critical`. The ticker should feel alive — never the same twice.

- **Difficulty:** Easy (string array + random selection + timer)
- **Battery Impact:** None
- **Wow Factor:** ★★★★

### 2.5 Screen Transitions — Horizontal Wipe

When navigating between screens, use a horizontal wipe transition (new screen slides in from the right, old screen slides out left). This mimics switching between displays on a mission control console. LVGL supports screen transitions natively with `lv_scr_load_anim()`. Back button reverses the direction (slides right).

- **Difficulty:** Easy (LVGL built-in)
- **Battery Impact:** None
- **Wow Factor:** ★★

### 2.6 Status Bar Redesign

Redesign the top status bar to look like a spacecraft telemetry strip: `[▓▓▓▓░] 72% | WiFi ◉ | MET 02:14:07 | ★ 3,450`. Battery as a segmented fuel gauge, WiFi as a signal icon, MET clock, and the player's best Lander score. All in monospace cyan on dark background. Always visible, always informative.

- **Difficulty:** Medium (layout + real-time updates)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 2.7 Monospace Typography for Data

Use the existing Montserrat for headings and labels, but switch all numeric data (scores, MET, coordinates, fuel readouts) to a monospace font. Numbers that don't jump around when digits change feel dramatically more "instrument panel." LVGL supports multiple fonts per screen. A small 14px monospace font is ~4KB of flash.

- **Difficulty:** Easy (add font, apply to number labels)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 2.8 Color-Coded Alert System

Standardize a NASA-style color language across all UI: Cyan (#00e5ff) = nominal/info, Green (#00c853) = success/GO, Amber (#ffab00) = warning/caution, Red (#ff1744) = critical/failure. Apply consistently to boot checks, game HUD, WiFi status, battery warnings, ticker messages. When battery drops below 15%, the status bar fuel gauge turns amber. Below 5%, red. This is how real mission control works.

- **Difficulty:** Easy (define palette, apply to existing widgets)
- **Battery Impact:** None
- **Wow Factor:** ★★


---

## 3. Social & Conference Features

A conference badge that doesn't encourage human interaction is just a screen on a lanyard. These features give people a reason to walk up to strangers and say "hey, what's your badge doing?"

### 3.1 Crew Roster — Badge Discovery

Badges periodically broadcast a short WiFi beacon (ESP-NOW or mDNS) with their callsign (chosen at check-in). A "Crew Roster" screen shows nearby badges you've discovered, sorted by first-seen time. Think Pokédex but for conference attendees. "You've encountered 47 of 300 crew members." People will actively seek out the ones they're missing.

- **Difficulty:** Hard (ESP-NOW broadcast + discovery + storage)
- **Battery Impact:** Medium (periodic WiFi scans)
- **Wow Factor:** ★★★★★

### 3.2 Callsign Selection

At first boot or check-in, the player picks a callsign from a curated list of space-themed names: `EAGLE`, `COLUMBIA`, `ODYSSEY`, `AQUARIUS`, `CHALLENGER`, `ENDEAVOUR`, etc. — all real spacecraft names. Or they type a custom one (≤10 chars) using an on-screen keyboard. This callsign appears on leaderboards, crew roster, and multiplayer lobbies. It's your identity for the day.

- **Difficulty:** Medium (on-screen keyboard + SD persistence)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 3.3 Achievement Trading

When two badges are in close proximity (detected via ESP-NOW RSSI), they can initiate a "docking sequence" — a fun animation of two spacecraft approaching each other. On successful dock, they exchange one random achievement badge each. This encourages people to meet as many attendees as possible. "I need the Apollo 13 achievement — anyone have it?"

- **Difficulty:** Hard (proximity detection + data exchange protocol + UI)
- **Battery Impact:** Medium
- **Wow Factor:** ★★★★★

### 3.4 Check-In Gamification

The existing check-in feature gets a space mission wrapper. Checking in at the conference = "Launch commit." Checking in at specific talks = "Orbital insertion at [talk name]." Checking in at the after-party = "Lunar landing." Each check-in awards mission patches (achievements) and XP. A "Mission Log" screen shows your journey through the conference as a timeline of mission events.

- **Difficulty:** Medium (server integration + UI for mission log)
- **Battery Impact:** Low (only on check-in events)
- **Wow Factor:** ★★★★

### 3.5 QR Code Badge Card

A "My Card" screen displays a QR code containing the attendee's callsign and a URL to their profile (or just a vCard with name/handle). Quick way to exchange contact info without fumbling with phones. The QR code is rendered locally — no network needed. Frame it with a mission patch border.

- **Difficulty:** Medium (QR generation library on ESP32)
- **Battery Impact:** None (static screen)
- **Wow Factor:** ★★★

### 3.6 Distress Beacon

A fun "SOS" feature: hold a button combo to broadcast a distress signal. Nearby badges see `⚠ DISTRESS BEACON: [callsign] requests assistance` with a flashing amber LED. Use case: "I'm lost, where's the CTF room?" or "I need help with a challenge." Silly but memorable. Could also be used as a "find my friend" ping.

- **Difficulty:** Medium (ESP-NOW broadcast + receive handler + UI)
- **Battery Impact:** Low (event-driven)
- **Wow Factor:** ★★★★


---

## 4. Gamification Beyond Lander

The Lander game is the centerpiece, but one game gets stale after a few hours. These ideas extend engagement across the entire conference day.

### 4.1 Achievement System — Mission Patches

A collection of ~20 unlockable "mission patches" (small pixel art icons) earned through various activities: `First Landing` (complete a game), `Eagle Scout` (score > 5000), `Apollo 13` (survive with < 1% fuel), `Neil's Ghost` (beat the dev's score), `Social Butterfly` (discover 25 badges), `Marathon` (play 10 games), `Speedrunner` (land in < 30 seconds), `Perfect Landing` (velocity < 0.5 at touchdown), `Night Owl` (play after 10pm). A "Mission Patches" screen shows collected vs. locked patches. This is the meta-game that ties everything together.

- **Difficulty:** Medium (achievement tracker + unlock conditions + patch art + SD storage)
- **Battery Impact:** None
- **Wow Factor:** ★★★★★

### 4.2 Hourly Challenges

Every hour, the game server pushes a new challenge: "Land on the leftmost pad," "Land with exactly 25% fuel remaining," "Achieve the slowest safe landing." Challenges appear in the ticker and on a dedicated screen. Completing them awards bonus XP and a unique hourly patch. This creates a rhythm to the day and gives people a reason to keep coming back to the game.

- **Difficulty:** Hard (server-side challenge system + badge client + validation)
- **Battery Impact:** Low (periodic server check)
- **Wow Factor:** ★★★★★

### 4.3 Ghost Replays

After completing a run, your input sequence is saved (~200 bytes). On the next run, a translucent "ghost" lander replays your previous best attempt alongside your live run. Racing your own ghost is addictive. Could also download the current #1 player's ghost from the server — now you're racing the best player at the conference.

- **Difficulty:** Hard (input recording + replay sync + dual rendering)
- **Battery Impact:** None
- **Wow Factor:** ★★★★★

### 4.4 Mission Control Jumbotron Leaderboard

The game server feeds a live leaderboard to a projector display ("Mission Control") in the main conference area. Top 10 scores update in real-time with callsigns, scores, and landing replays. When someone takes #1, the jumbotron plays a celebration animation and the player's badge LEDs go gold. Seeing your callsign on a giant screen in front of 300 people is an unforgettable moment.

- **Difficulty:** Medium on badge side (server already supports it), Hard for jumbotron display
- **Battery Impact:** None (server push)
- **Wow Factor:** ★★★★★

### 4.5 Difficulty Progression

Lock Medium difficulty until the player lands successfully on Easy. Lock Hard until they land on Medium. This gives a natural progression arc through the day. Each difficulty unlock triggers a "FLIGHT DIRECTOR: Crew cleared for [Medium/Hard] operations" message and a brief LED celebration.

- **Difficulty:** Easy (gate check + SD flag)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 4.6 Lander Cosmetics

Unlock alternate lander skins by earning achievements: a gold lander for #1 score, a red lander for 10 crashes, a rainbow lander for discovering 50 badges, a skull lander for CTF progress. Purely cosmetic, but people love customization. Skins are just palette swaps on the existing sprite — minimal memory cost.

- **Difficulty:** Medium (skin system + unlock triggers + renderer changes)
- **Battery Impact:** None
- **Wow Factor:** ★★★★

### 4.7 Speed Run Timer

A dedicated speed run mode with a precise millisecond timer displayed prominently. Measures from first thrust input to successful landing. Separate leaderboard category. Speed runners are a vocal, competitive community — give them a stage and they'll generate hype all day.

- **Difficulty:** Easy (timer overlay + separate scoreboard category)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 4.8 Tournament Bracket Mode

The game server runs a structured tournament: 16 players, single elimination, best-of-3 rounds. Brackets displayed on the jumbotron. Players get push notifications on their badge when their match is up: `FLIGHT: [callsign], report to launch pad. Match begins in T-minus 2:00`. Winner gets a unique "Tournament Champion" achievement that no one else can earn.

- **Difficulty:** Hard (server bracket logic + scheduling + badge notifications)
- **Battery Impact:** Low
- **Wow Factor:** ★★★★★


---

## 5. LED Integration

Six NeoPixels isn't many, but six bright, programmable LEDs on someone's chest at a dark conference are incredibly visible. Every LED state should mean something.

### 5.1 Thrust Glow

During gameplay, the bottom 2 LEDs glow orange-to-white proportional to thrust intensity. When the player touches the thrust zone, the LEDs flare up. When they release, the LEDs fade out over ~200ms. This creates a visceral connection between touch input and the physical badge. People watching over your shoulder see the thrust before they see the screen.

- **Difficulty:** Easy (map thrust value to LED brightness/color)
- **Battery Impact:** Medium (bright LEDs during active gameplay)
- **Wow Factor:** ★★★★

### 5.2 Landing Celebration

On a successful landing: all 6 LEDs flash green 3 times, then settle into a slow green pulse for 5 seconds. On a crash: rapid red flash, then fade to black. On a new high score: gold sparkle animation (random LEDs flash white/gold). The LEDs become the badge's emotional response to your performance.

- **Difficulty:** Easy
- **Battery Impact:** Low (brief bursts)
- **Wow Factor:** ★★★★

### 5.3 Fuel Gauge LEDs

During gameplay, the 6 LEDs double as a fuel gauge. Full fuel = all 6 green. As fuel depletes, LEDs turn off from the edges inward. Below 20%: remaining LEDs turn amber. Below 10%: red and blinking. This gives players critical game info in their peripheral vision without looking away from the screen.

- **Difficulty:** Easy (map fuel percentage to LED count + color)
- **Battery Impact:** Low (dim LEDs)
- **Wow Factor:** ★★★★★

### 5.4 Multiplayer Team Colors

In multiplayer mode, each player is assigned a team color. Their badge LEDs glow that color at low brightness throughout the match. Walk around the conference and you can see who's on which team. Creates instant tribal identity. "Hey, you're blue team too!"

- **Difficulty:** Easy (server assigns color, badge sets LEDs)
- **Battery Impact:** Low
- **Wow Factor:** ★★★★

### 5.5 Proximity Pulse

When two badges detect each other via ESP-NOW, both briefly pulse their LEDs in cyan — a subtle "hello" that happens automatically. If you've never encountered this badge before (new crew roster entry), the pulse is brighter and longer. You'll start noticing the pulses in crowded hallways and know: "someone new is nearby."

- **Difficulty:** Medium (tied to crew roster discovery system)
- **Battery Impact:** Low (brief, infrequent)
- **Wow Factor:** ★★★★★

### 5.6 Achievement Unlock Fanfare

When an achievement unlocks, the LEDs do a unique 2-second animation: a rainbow sweep from left to right, ending on gold. This is Pavlovian — after a few unlocks, people will start chasing that rainbow sweep. It's also visible to people nearby, who will ask "what did you just unlock?"

- **Difficulty:** Easy
- **Battery Impact:** Low
- **Wow Factor:** ★★★★

### 5.7 Heartbeat Status Indicator

On the main menu, a single LED does a slow double-pulse (like a heartbeat) in dim cyan to indicate the badge is alive and connected. If WiFi drops, it switches to a slow amber single-pulse. If battery is critical, dim red. One LED, always on, always communicating status. The other 5 stay off to save power.

- **Difficulty:** Easy
- **Battery Impact:** Very Low (1 dim LED)
- **Wow Factor:** ★★


---

## 6. Audio Design

A single piezo buzzer. No polyphony, no WAV playback. But a well-designed beep is worth a thousand pixels. Every sound should be short, distinctive, and satisfying.

### 6.1 Boot Sequence Tones

Each boot check gets a rising tone: 440Hz → 554Hz → 659Hz → 880Hz (A4 → C#5 → E5 → A5, an A-major arpeggio). Each tone is 80ms with a 40ms gap. The final "LAUNCH COMMIT" gets a triumphant two-tone: 880Hz→1760Hz (octave jump). Total boot audio: ~1 second. Sets the tone (literally) for the entire experience.

- **Difficulty:** Easy (tone sequence with delays)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 6.2 Menu Navigation Clicks

Soft 50ms clicks at 2000Hz when selecting menu items. A slightly lower 1500Hz tone when pressing "back." Different enough to feel directional — forward is higher, backward is lower. Keep them quiet and short. Nobody wants to be the person whose badge is beeping during a talk.

- **Difficulty:** Easy
- **Battery Impact:** None
- **Wow Factor:** ★

### 6.3 Thrust Audio

While thrusting, play a continuous low tone (150Hz) that rises in pitch with thrust duration — simulating engine strain. When thrust is released, a quick descending chirp (200Hz → 80Hz, 100ms). This is the most important game sound: it makes the lander feel physical. Pair with the thrust LED glow for a multi-sensory experience.

- **Difficulty:** Easy (tone on touch, off on release)
- **Battery Impact:** None
- **Wow Factor:** ★★★★

### 6.4 Altitude Warning

Below 50m altitude, a repeating beep at 1000Hz that increases in frequency as you approach the ground — exactly like a radar altimeter callout. 1 beep/sec at 50m, 2/sec at 25m, 4/sec at 10m, continuous tone at 5m. Pilots and aviation nerds will immediately recognize this. Everyone else will feel the tension.

- **Difficulty:** Easy (timer-based beep rate tied to altitude)
- **Battery Impact:** None
- **Wow Factor:** ★★★★

### 6.5 Landing Jingles

Successful landing: a quick 5-note ascending melody (C5-E5-G5-C6-E6, 80ms each) — triumphant and bright. Crash: a descending chromatic slide from 800Hz to 100Hz over 300ms — the universal sound of failure. New high score: the landing jingle followed by a 3-note fanfare (G5-C6-E6, longer notes). These become the emotional punctuation of every run.

- **Difficulty:** Easy
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 6.6 Achievement Unlock Sound

A distinctive 4-note jingle that plays only on achievement unlock: two quick ascending notes, a pause, then a high sustained note (think: the Zelda "item get" but in buzzer form). This sound should be unique enough that after hearing it once, you know exactly what it means from across the room.

- **Difficulty:** Easy
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 6.7 Countdown Timer Tones

For tournament matches and hourly challenges: a countdown sequence at T-minus 10 seconds. One beep per second (1000Hz, 100ms) from T-10 to T-3, then three rapid higher beeps (1500Hz) at T-3, T-2, T-1, then a long tone (2000Hz, 500ms) at T-0. Classic rocket launch cadence.

- **Difficulty:** Easy
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 6.8 Silent Mode

A global mute toggle accessible from the system menu (and via a quick button combo — e.g., hold Back for 2 seconds). When muted, a small 🔇 icon appears in the status bar. Respects the conference environment. Talks are sacred.

- **Difficulty:** Easy (global flag checked before every tone() call)
- **Battery Impact:** None (saves a tiny bit)
- **Wow Factor:** ★ (but essential)


---

## 7. Easter Eggs & Delight

The best conference badges are the ones people are still discovering secrets in on the drive home. Easter eggs reward curiosity — the core trait of every security conference attendee.

### 7.1 Konami Code

On the main menu, tap the screen edges in the Konami pattern (↑↑↓↓←→←→ mapped to top-top-bottom-bottom-left-right-left-right). Unlocks a hidden "God Mode" bling pattern: all 6 LEDs cycle through every color at full brightness while the screen displays `CHEAT ACTIVATED` in giant green text for 3 seconds. Also unlocks a secret lander skin (the lander is now a tiny pirate ship — a nod to BadgePirates).

- **Difficulty:** Easy (input sequence detector + flag)
- **Battery Impact:** None
- **Wow Factor:** ★★★★

### 7.2 Apollo 11 Transcript Mode

Hidden in the credits screen (tap the BadgePirates logo 5 times): the ticker switches to replaying the actual Apollo 11 landing transcript in real-time pacing. "60 seconds." ... "EAGLE: Houston, Tranquility Base here. The Eagle has landed." ... "HOUSTON: Roger, Tranquility, we copy you on the ground." Runs for ~3 minutes. Goosebumps guaranteed.

- **Difficulty:** Easy (stored string array + timed display)
- **Battery Impact:** None
- **Wow Factor:** ★★★★★

### 7.3 1202 Alarm

If the badge encounters a real software error (WiFi timeout, SD read failure, etc.), instead of a generic error message, display `1202 ALARM` in amber — the same program alarm that nearly aborted the Apollo 11 landing. Include the actual error details below in smaller text. A fun way to handle errors that turns bugs into features.

- **Difficulty:** Easy (error handler wrapper)
- **Battery Impact:** None
- **Wow Factor:** ★★★★

### 7.4 Hidden Game: Asteroids

Buried in the system menu (tap "System Info" 7 times rapidly): a simple Asteroids clone. Ship rotates and thrusts with the same touch zones as Lander. Asteroids are just circles that split when shot. No scoring, no saves — just a hidden toy. The fact that it exists at all is the point.

- **Difficulty:** Hard (second game engine, even if simple)
- **Battery Impact:** None
- **Wow Factor:** ★★★★★

### 7.5 Famous Space Quotes

Every boot shows a random space quote on the splash screen, below the BSidesKC logo:

- *"That's one small step for man..."* — Neil Armstrong
- *"Houston, we've had a problem."* — Jack Swigert
- *"The Earth is the cradle of humanity, but one cannot live in a cradle forever."* — Tsiolkovsky
- *"Ad astra per aspera."* — Kansas state motto (full version)
- *"I didn't feel like a giant. I felt very, very small."* — Buzz Aldrin
- *"Failure is not an option."* — Gene Kranz

~20 quotes, randomly selected. Costs almost nothing, adds personality to every power-on.

- **Difficulty:** Easy (string array + random index)
- **Battery Impact:** None
- **Wow Factor:** ★★★

### 7.6 Secret Bling: Mission Patch

A hidden LED mode unlocked by earning 10+ achievements: the 6 LEDs display a slow, repeating color sequence that encodes "AD ASTRA" in Morse code using cyan flashes. Only people who know Morse (or who look it up) will decode it. A secret within a secret.

- **Difficulty:** Easy (Morse timing array + LED playback)
- **Battery Impact:** Low
- **Wow Factor:** ★★★★

### 7.7 Developer Credits Crawl

In the credits screen, after the normal credits scroll, wait 10 seconds. A Star Wars-style text crawl begins, telling the story of the badge development in dramatic fashion: "In a city on the plains... a band of pirates forged silicon and code... badges were lost to customs... but the crew would not be stopped..." Scrolls slowly up the screen in perspective (or just straight up — we have 240 pixels to work with). Ends with "FIN" and a tiny lander landing on the bottom of the screen.

- **Difficulty:** Medium (smooth text scroll + story writing)
- **Battery Impact:** None
- **Wow Factor:** ★★★★★

### 7.8 The Answer

If the badge's MET clock hits exactly `04:02:00` (4 hours, 2 minutes — "42"), all LEDs flash white once and the ticker displays: `DEEP THOUGHT: The answer is 42.` Blink and you miss it.

- **Difficulty:** Easy
- **Battery Impact:** None
- **Wow Factor:** ★★★


---

## 8. Conference Integration

The badge isn't just a toy — it's a conference tool. These features make it genuinely useful while staying on theme.

### 8.1 Talk Reminders with Countdown

The schedule viewer gets a "Set Reminder" button on each talk. At T-minus 5 minutes, the badge buzzes, LEDs flash cyan, and the ticker shows `FLIGHT: [Talk Title] launching in T-minus 5:00 — [Room]`. At T-minus 1, another buzz. Never miss a talk you wanted to see. The reminder uses the NASA countdown cadence from section 6.7.

- **Difficulty:** Medium (timer system + schedule integration)
- **Battery Impact:** Low (event-driven)
- **Wow Factor:** ★★★★

### 8.2 Live CTF Scoreboard

A dedicated screen that polls the CTF scoreboard API every 60 seconds and displays the top 10 teams with scores. Your team (if registered) is highlighted in green. Score changes trigger a brief LED flash. Keeps the competitive energy visible all day without needing to find a laptop.

- **Difficulty:** Medium (HTTP client + JSON parsing + UI)
- **Battery Impact:** Medium (periodic WiFi requests)
- **Wow Factor:** ★★★★

### 8.3 Hallway Track Beacon

A "Where's the party?" screen that shows active social zones. Badges in the same area form ad-hoc clusters via ESP-NOW. The screen shows: `Lobby: 23 crew | Track A: 45 crew | CTF Room: 12 crew | Hallway: 8 crew`. Helps you find where people are congregating. Conference organizers can also push location labels via the server.

- **Difficulty:** Hard (distributed counting via ESP-NOW + location mapping)
- **Battery Impact:** Medium
- **Wow Factor:** ★★★★★

### 8.4 After-Party Countdown

Starting at 5:00 PM, a countdown timer appears in the status bar: `PARTY T-minus 02:34:17`. As it approaches zero, the ticker messages get increasingly excited: `CAPCOM: Post-mission debrief approaching` → `FLIGHT: All stations, prepare for splashdown` → `CAPCOM: 🎉 SPLASHDOWN. Report to after-party.` At T-0, all LEDs go into party mode (rainbow cycle at full brightness).

- **Difficulty:** Easy (hardcoded time + countdown math)
- **Battery Impact:** Low (LEDs only at party time)
- **Wow Factor:** ★★★★

### 8.5 OTA Update Station

A physical "firmware update station" at the conference (a laptop running the OTA server). Badges connect to a dedicated WiFi AP and pull updates. The update screen shows a NASA-style progress display: `UPLOADING FLIGHT SOFTWARE... Block 147/512 [▓▓▓▓▓▓░░░░] 28%`. This lets the team push bug fixes, new challenges, and surprise features throughout the day. Morning firmware is different from afternoon firmware.

- **Difficulty:** Easy (OTA already exists, just polish the UI)
- **Battery Impact:** Medium (WiFi during update)
- **Wow Factor:** ★★★

### 8.6 Speaker Rating — Mission Debrief

After a talk's scheduled end time, if you set a reminder for it, the badge prompts: `MISSION DEBRIEF: Rate [Talk Title]`. Three big touch buttons: 🚀 (great), 🛰️ (good), 💥 (rough landing). One tap, anonymous, sent to the server. Speakers get aggregated feedback. Simple enough that people actually do it. The space metaphors keep it fun and non-judgmental.

- **Difficulty:** Medium (server endpoint + timed prompt + UI)
- **Battery Impact:** Low
- **Wow Factor:** ★★★

### 8.7 Conference WiFi Auto-Connect

On first boot, the badge auto-connects to the conference WiFi (SSID and password baked into firmware). No manual WiFi setup needed for 90% of attendees. A "WiFi" menu option still exists for connecting to other networks. The boot sequence shows `Comms link [OK] — BSidesKC-2026` to confirm. Removes the single biggest friction point for every networked feature on this list.

- **Difficulty:** Easy (hardcode SSID/password, auto-connect on boot)
- **Battery Impact:** None (WiFi was going to be on anyway)
- **Wow Factor:** ★★ (invisible when it works, infuriating when it doesn't)

### 8.8 Mission Control Jumbotron Feed

The game server pushes a live data feed to the venue's projector display: real-time leaderboard, live game spectating, tournament brackets, hourly challenge status, total badges online, total landings/crashes conference-wide, and a scrolling ticker of notable events ("EAGLE just set a new high score!", "50th crash of the hour"). This is the heartbeat of the badge experience — it makes the invisible network visible to everyone in the room.

- **Difficulty:** Hard (web dashboard + server events + display design)
- **Battery Impact:** None (server-side)
- **Wow Factor:** ★★★★★

---

## Priority Matrix

If time is limited, here's what to build first — maximum impact for minimum effort:

### Must-Have (Easy wins, huge payoff)
| Idea | Section | Difficulty | Wow |
|------|---------|------------|-----|
| Boot Sequence Enhancement | 2.2 | Easy | ★★★★ |
| Comms Chatter Ticker | 2.4 | Easy | ★★★★ |
| Fuel Gauge LEDs | 5.3 | Easy | ★★★★★ |
| Thrust Glow + Audio | 5.1 / 6.3 | Easy | ★★★★ |
| Landing Celebrations (LED + Audio) | 5.2 / 6.5 | Easy | ★★★★ |
| Altitude Warning Audio | 6.4 | Easy | ★★★★ |
| Famous Space Quotes | 7.5 | Easy | ★★★ |
| 1202 Alarm Error Handler | 7.3 | Easy | ★★★★ |
| After-Party Countdown | 8.4 | Easy | ★★★★ |
| MET Clock | 2.1 | Easy | ★★★ |
| Silent Mode | 6.8 | Easy | ★ |

### Should-Have (Medium effort, strong impact)
| Idea | Section | Difficulty | Wow |
|------|---------|------------|-----|
| Achievement System | 4.1 | Medium | ★★★★★ |
| Callsign Selection | 3.2 | Medium | ★★★ |
| Status Bar Redesign | 2.6 | Medium | ★★★ |
| Talk Reminders | 8.1 | Medium | ★★★★ |
| Check-In Gamification | 3.4 | Medium | ★★★★ |
| Lander Cosmetics | 4.6 | Medium | ★★★★ |
| Developer Credits Crawl | 7.7 | Medium | ★★★★★ |
| Live CTF Scoreboard | 8.2 | Medium | ★★★★ |

### Stretch Goals (High effort, legendary impact)
| Idea | Section | Difficulty | Wow |
|------|---------|------------|-----|
| Crew Roster Discovery | 3.1 | Hard | ★★★★★ |
| Ghost Replays | 4.3 | Hard | ★★★★★ |
| Tournament Brackets | 4.8 | Hard | ★★★★★ |
| Achievement Trading | 3.3 | Hard | ★★★★★ |
| Mission Control Jumbotron | 8.8 | Hard | ★★★★★ |
| Hidden Asteroids Game | 7.4 | Hard | ★★★★★ |

---

*"Ad astra per aspera" — To the stars, through difficulties.*

*Let's make sure nobody forgets this badge.*
