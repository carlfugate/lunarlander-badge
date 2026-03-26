# Content Strategy: Lunar Lander Badge Project

> An IT architect who never writes code used AI to build production embedded firmware for a security conference badge. Here's how to tell that story.

---

## 1. LinkedIn Post Angles

### Post A: "The Confession" (Personal Vulnerability Hook)

**Hook:** I'm not a developer. I haven't written production code in my career. Last month I shipped 15,000+ lines of C++ firmware for a conference badge — and I never typed a single line of it.

**Key Narrative:** As Cabinet Chair for Cloud Architecture and Enterprise Architecture at Netsmart, my job is designing systems, not coding them. But when BSidesKC needed conference badges and the original hardware got stuck in customs last year, I decided to try something different. I used AI coding agents (Kiro CLI with Claude) as my development team — directing the work like a tech lead while the AI wrote every line of C++, Python, and HTML. Two sessions. A full game engine, animated screensaver, Bluetooth social features, achievement system, and 47 unit tests. The badge runs a Lunar Lander game on an ESP32-S3 with a touchscreen, NeoPixels, and a buzzer. Ad Astra.

**Call to Action:** What's the most ambitious thing you've built with AI tools? I'm genuinely curious — drop it in the comments.

**Hashtags:** #AI #BadgeLife #BSidesKC #EnterpriseArchitecture #AIAssistedDevelopment #OpenSource

**Engagement Potential:** HIGH — The vulnerability of "I'm not a developer" combined with an impressive outcome creates strong cognitive dissonance. People will share this to make a point about AI, regardless of which side they're on.

---

### Post B: "The Supervisor Pattern" (Technical Leadership Angle)

**Hook:** I managed an AI the same way I manage architects — and it shipped production firmware.

**Key Narrative:** Enterprise architects don't write code. We define patterns, review decisions, and unblock teams. So when I used AI agents to build an ESP32 conference badge, I ran it exactly like a real project: created GitHub issues, assigned parallel workstreams, reviewed output, and iterated on failures. The AI handled C++ firmware, Python test automation, HTML mockups, documentation, and git operations. I handled architecture decisions, hardware testing, and quality gates. We closed 65 of 80 GitHub issues across two working sessions. The interesting part isn't that AI can write code — it's that existing management patterns transfer directly to AI supervision.

**Call to Action:** If you lead technical teams, you already have the skills to direct AI agents. The question is whether your organization is ready for that conversation.

**Hashtags:** #TechLeadership #AI #EnterpriseArchitecture #FutureOfWork #AgenticAI #ProjectManagement

**Engagement Potential:** HIGH — IT leaders will see themselves in this. Developers will have opinions. Both reactions drive engagement.

---

### Post C: "The Bug Hunt" (War Story)

**Hook:** The AI wrote the code. The badge crashed. Then the real work started.

**Key Narrative:** Building firmware with AI sounds magical until you flash it to real hardware and it reboots in a loop. The Lunar Lander badge project taught me that AI-generated code has the same problems as human-generated code — LVGL lifecycle bugs, FreeRTOS task safety issues, memory leaks under sustained load. The difference? I built a serial test protocol (BSTP v1.0), automated soak testing with crash and hang detection, and used AI to decode stack traces and fix its own bugs. The cycle was: flash, crash, decode backtrace, fix, repeat. We found and fixed multiple crash bugs that only appeared after hours of continuous operation. AI doesn't eliminate debugging. It changes who's holding the debugger.

**Call to Action:** Anyone else doing hardware-in-the-loop testing with AI-generated firmware? I'd love to compare notes.

**Hashtags:** #EmbeddedSystems #Debugging #AI #FirmwareDevelopment #QualityEngineering #BSidesKC

**Engagement Potential:** MEDIUM-HIGH — Engineers will respect the honesty about crashes. The "AI fixing its own bugs" angle is novel and shareable.

---

### Post D: "The Numbers" (Data-Driven Proof)

**Hook:** 2 sessions. 0 lines of code written by hand. Here's what shipped:

**Key Narrative:**
- Full Lunar Lander game engine (physics, terrain gen, collision detection, 4 play modes)
- 9-scene animated screensaver (warp speed → solar system → Apollo descent → matrix rain)
- BLE badge-to-badge social features (presence detection, crew roster, messaging)
- Achievement system, callsign selection, conference schedule, OTA updates
- NASA Mission Control HUD theme across 15+ screens
- 12 LED animation patterns
- 47 unit tests + automated soak testing framework
- ~80 GitHub issues tracked, ~65 closed

All running on an ESP32-S3 with a 320x240 touchscreen, 6 NeoPixels, and a buzzer. Built for BSidesKC 2026. Open source. I'm an enterprise architect, not a developer. AI was the entire engineering team.

**Call to Action:** The repo is public: github.com/carlfugate/lunarlander-badge. Star it, fork it, tell me what I got wrong.

**Hashtags:** #AI #OpenSource #BadgeLife #ESP32 #BSidesKC #BuildInPublic

**Engagement Potential:** MEDIUM — Lists perform well on LinkedIn. The contrast between "2 sessions" and the feature list is the hook. Including the repo link will drive GitHub traffic.

---

### Post E: "The Enterprise Architect's Secret" (Role Identity)

**Hook:** Enterprise architects are about to become the most dangerous people in your organization.

**Key Narrative:** We already understand system design, integration patterns, failure modes, and quality gates. We think in abstractions. We manage complexity for a living. Now give us AI coding agents. I used that exact skill set to build a conference badge — an ESP32 with a game engine, Bluetooth social features, animated screensavers, and a full test suite. I didn't learn C++. I didn't learn embedded development. I applied architecture thinking to a new kind of team. The gap between "I can design it" and "I can ship it" just collapsed.

**Call to Action:** Fellow architects — what would you build if the only constraint was your imagination and not your coding ability?

**Hashtags:** #EnterpriseArchitecture #CloudArchitecture #AI #Innovation #FutureOfWork #DigitalTransformation

**Engagement Potential:** HIGH — Architects will feel seen and empowered. Developers will debate whether this is good or terrifying. Both reactions are engagement.

---

### Post F: "The Badge Culture" (Community / Niche)

**Hook:** In the security conference world, your badge IS your resume. This year, mine runs a Lunar Lander game.

**Key Narrative:** #BadgeLife is a subculture at security conferences where custom electronic badges are art, engineering, and social currency. BSidesKC 2026 badges were originally designed by BadgePirates (bplabs.tech), but last year's badges got stuck in customs and barely got used. This year, we're making sure the firmware is worth the wait. The badge runs a full Lunar Lander game, has a 9-scene space-themed screensaver, does BLE badge-to-badge communication, and even has a bot API for AI competitions on the game server. The project motto is "Ad Astra" — to the stars — which is also the Kansas state motto. Built with AI. Open source. See you in KC.

**Call to Action:** If you're coming to BSidesKC, the badge firmware is open source — start hacking before you even arrive. github.com/carlfugate/lunarlander-badge

**Hashtags:** #BadgeLife #BSidesKC #InfoSec #HackerCulture #AdAstra #KansasCity #OpenSource

**Engagement Potential:** MEDIUM — Niche but passionate audience. Will circulate heavily in security conference circles. Good for building anticipation before the event.

---

### Post G: "The Parallel Agents" (AI Power Users)

**Hook:** I ran 4 AI coding agents in parallel — like a dev team that never needs a standup.

**Key Narrative:** The Lunar Lander badge project wasn't one AI conversation. It was parallel subagents implementing different features simultaneously — one on the game engine, one on BLE social features, one on the screensaver system, one on the test framework. I coordinated them the way a tech lead coordinates a sprint. When one agent's work created a conflict with another's, I resolved it. When hardware testing revealed a crash, I routed the stack trace to the right agent. This isn't "asking ChatGPT to write code." This is agentic software development with a human architect in the loop.

**Call to Action:** The tooling for multi-agent development is still early. What patterns are you finding that work?

**Hashtags:** #AgenticAI #AIEngineering #DeveloperProductivity #FutureOfWork #Kiro #BuildInPublic

**Engagement Potential:** MEDIUM-HIGH — The AI power-user community is growing fast and hungry for real workflow examples. This positions Carl as a practitioner, not a theorist.

---

## 2. Blog Post Concepts

### Blog A: "I'm Not a Developer: How an Enterprise Architect Shipped Embedded Firmware with AI"

**Target Audience:** IT leaders, enterprise architects, technical managers, AI-curious professionals

**Outline:**
1. **Who I am and what I don't do** — Cloud/Enterprise Architecture at Netsmart. I design systems. I don't write code. That's not false modesty; it's my actual job description.
2. **The BSidesKC badge problem** — Conference badges got stuck in customs last year. This year needed to be different. What the hardware looks like (ESP32-S3, touchscreen, NeoPixels, buzzer).
3. **The supervisor pattern** — How I treated AI agents like a development team. GitHub issues, parallel workstreams, architecture decisions, quality gates. The same skills I use daily, applied to a radically different "team."
4. **What we built** — Walk through the feature set with screenshots/photos. Game engine, screensaver, BLE social, achievements, OTA updates. The scope that makes people say "wait, really?"
5. **Where it broke** — Honest accounting of crashes, LVGL lifecycle bugs, FreeRTOS issues. The soak testing framework. Why hardware-in-the-loop testing matters and AI can't skip it.
6. **What this means for IT professionals** — The gap between architect and builder is closing. You don't need to learn to code. You need to learn to supervise AI that codes. Those are different skills, and you might already have them.
7. **Try it yourself** — Link to the open source repo. Invitation to contribute. Ad Astra.

**Key Takeaway:** The skills that make someone a good technical leader — system thinking, quality standards, iterative problem-solving — transfer directly to AI-supervised development. The barrier to building just dropped dramatically for an entire class of IT professionals.

**Where to Publish:** LinkedIn Article (primary, for professional network reach) + cross-post to Medium (for broader discovery). Consider a condensed version for dev.to to reach the developer audience.

---

### Blog B: "Building a Serial Test Protocol for AI-Generated Firmware: BSTP v1.0"

**Target Audience:** Embedded developers, QA engineers, AI tooling builders, hardware hackers

**Outline:**
1. **The problem** — AI writes code fast. Hardware crashes faster. You need automated testing that bridges the gap between "it compiles" and "it runs for 8 hours without crashing."
2. **BSTP v1.0 design** — The serial test protocol: what it sends, what it expects, how it handles timeouts and hangs. Design decisions and tradeoffs.
3. **The soak test framework** — Overnight testing with crash detection, hang detection, and automatic log capture. How it found bugs that manual testing missed.
4. **MCP server for hardware testing** — Using Model Context Protocol to let AI agents interact with physical hardware through a structured interface. The Kiro skill that ties it together.
5. **Bugs we caught** — Real examples: LVGL lifecycle issues, FreeRTOS task safety, memory behavior under sustained load. Stack traces and fixes.
6. **Making it reusable** — The framework is designed to be backportable. How other badge projects or embedded teams could adopt it.

**Key Takeaway:** AI-generated firmware needs the same (or more) rigorous testing as human-written firmware. Automated hardware-in-the-loop testing isn't optional — and the testing framework itself can be AI-assisted.

**Where to Publish:** dev.to (primary, technical audience) + personal blog. Consider submitting to Hackaday for the hardware/maker crossover audience.

---

### Blog C: "Ad Astra: The Making of the BSidesKC 2026 Badge"

**Target Audience:** Security conference community, badge makers, #BadgeLife enthusiasts

**Outline:**
1. **Badge culture primer** — For the uninitiated: what #BadgeLife is, why security conferences have custom electronic badges, and why people care so much.
2. **The BSidesKC story** — Last year's badges stuck in customs. The BadgePirates hardware design (bplabs.tech). The decision to make this year's firmware unforgettable.
3. **Feature tour** — Lunar Lander gameplay, the 9-scene screensaver narrative (warp speed through the cosmos to lunar landing), BLE social features, the NASA Mission Control aesthetic. Photos and video.
4. **The AI twist** — How the firmware was built entirely with AI assistance. Brief version of the process for an audience that cares more about the result than the method.
5. **The bot API and AI competitions** — The game server's bot API. What it enables for the hacker community. Competitive AI Lunar Lander at a security conference.
6. **Open source and community** — The repo, how to contribute, how to hack your badge before and during the conference. Ad Astra.

**Key Takeaway:** BSidesKC 2026 badges are worth showing up for. The firmware is deep, the features are real, and the whole thing is open source and hackable.

**Where to Publish:** Personal blog + submit to Hackaday. Share in BSides Slack/Discord channels and security conference communities. Time publication for 4-6 weeks before the event.

---

### Blog D: "4 Agents, 1 Architect: Multi-Agent AI Development in Practice"

**Target Audience:** AI engineers, developer productivity enthusiasts, technical leaders exploring agentic workflows

**Outline:**
1. **Beyond the single prompt** — Why complex projects can't be built in one AI conversation. The need for parallel workstreams, shared context, and human coordination.
2. **The setup** — Kiro CLI with Claude, parallel subagents, GitHub as the coordination layer. How the workflow was structured.
3. **Coordination patterns** — How to split work across agents. When agents conflict. How to maintain architectural consistency when multiple agents are generating code.
4. **The human in the loop** — What the architect actually does: makes decisions the AI can't, tests on real hardware, resolves conflicts, maintains vision. The irreducible human role.
5. **What worked and what didn't** — Honest assessment. Where parallel agents saved massive time. Where they created integration headaches. What I'd do differently.
6. **Implications for teams** — If one architect can direct multiple AI agents to build a complex embedded system, what does that mean for team structure, hiring, and project planning?

**Key Takeaway:** Multi-agent AI development is real and productive today, but it requires a specific kind of human skill — the ability to think in systems, decompose problems, and maintain quality across parallel workstreams. That's architecture.

**Where to Publish:** LinkedIn Article (primary) + Medium. This is the thought leadership piece that positions Carl for speaking invitations.

---

## 3. Technical Talk / Lightning Talk Ideas

### Talk A: "I Can't Code: How an Enterprise Architect Built a Conference Badge with AI"

**Abstract:** An enterprise architect with zero firmware experience used AI coding agents to build a full-featured ESP32 conference badge — Lunar Lander game, BLE social features, animated screensavers, and 47 unit tests. This talk covers the supervisor pattern, hardware-in-the-loop testing with AI, and what happens when the gap between "architect" and "builder" disappears.

**Key Demo Moments:**
- Live badge demo: launch the Lunar Lander game, show the screensaver sequence, trigger a BLE interaction with a second badge
- Side-by-side: the GitHub issue Carl wrote vs. the code the AI produced
- The crash reel: actual stack traces from soak testing, and the AI-generated fixes
- Speed round: show the repo stats (lines of code, issues closed, test count) against the timeline

**Best Venue:** BSidesKC 2026 (obvious home game), but also strong for: regional BSides events, DevOpsDays, local tech meetups, AWS/cloud community days

**Format:** 25-minute talk or 45-minute talk with extended demo

---

### Talk B: "Ad Astra: Badge Firmware Speedrun" (Lightning Talk)

**Abstract:** From zero to a full game engine, 9-scene screensaver, BLE social features, and 47 tests — on an ESP32 conference badge — in two AI-assisted sessions. A 10-minute speedrun of the BSidesKC 2026 badge build, with live hardware demo.

**Key Demo Moments:**
- 60-second badge feature tour (game, screensaver, BLE, LEDs)
- The "wall of GitHub issues" — show the project board
- One good crash story in 90 seconds
- End with: "The repo is open. Hack your badge before the conference."

**Best Venue:** BSidesKC 2026 lightning talk track, local meetups, Hackaday Supercon lightning talks

**Format:** 10 minutes, fast-paced, heavy on visuals and live demo

---

### Talk C: "Testing AI-Generated Firmware: A Serial Protocol for Hardware-in-the-Loop QA"

**Abstract:** When AI writes your firmware, who tests it? This talk introduces BSTP v1.0, a serial test protocol for automated hardware-in-the-loop testing of AI-generated embedded code. Built for the BSidesKC 2026 badge project, the framework includes soak testing with crash/hang detection, MCP integration for AI-driven test execution, and a backportable architecture for other embedded projects.

**Key Demo Moments:**
- Live soak test running on the badge, showing serial protocol traffic
- A real crash caught by automated testing (log replay)
- MCP server demo: AI agent interacting with physical hardware through structured commands
- Before/after: manual testing vs. automated framework

**Best Venue:** Embedded systems conferences, testing/QA meetups, BSidesKC (technical track). Could also work at AI engineering conferences (focus on the MCP/agent angle).

**Format:** 25-minute technical talk

---

### Talk D: "The Architect's Unfair Advantage: Why System Thinkers Will Win the AI Era"

**Abstract:** Developers worry AI will replace them. Architects should be excited — the skills that define our role (system decomposition, integration patterns, quality gates, failure mode analysis) are exactly what's needed to supervise AI development teams. A case study from building an ESP32 conference badge with zero coding, using the same patterns that run enterprise architecture programs.

**Key Demo Moments:**
- Architecture diagram of the badge system vs. architecture diagram of an enterprise system — show the pattern match
- The parallel agent workflow mapped to a sprint board
- "Decisions AI couldn't make" — the moments where architecture judgment was irreplaceable

**Best Venue:** Enterprise architecture conferences (Gartner, Open Group), IT leadership events, cloud summits. Also strong as a keynote-style talk at regional tech conferences.

**Format:** 30-45 minute keynote-style talk, minimal code, heavy on narrative and visuals

---

## 4. Content Calendar

### Phase 1: Foundation (Now through January 2026)

| Timing | Content | Platform | Goal |
|--------|---------|----------|------|
| **Now** | Post A ("The Confession") or Post E ("The Enterprise Architect's Secret") | LinkedIn | Establish the narrative. First public mention of the project. Gauge which angle resonates. |
| **1 week later** | Post D ("The Numbers") | LinkedIn | Follow up with specifics. Drive traffic to the GitHub repo. |
| **2 weeks later** | Blog A ("I'm Not a Developer") | LinkedIn Article + Medium | Deep-dive for people who engaged with the posts. This becomes the canonical reference. |
| **Monthly** | Progress update post (new features, interesting bugs, community contributions) | LinkedIn | Keep the project visible. Build in public. |

### Phase 2: Build Momentum (February - March 2026)

| Timing | Content | Platform | Goal |
|--------|---------|----------|------|
| **Early Feb** | Blog C ("Ad Astra: The Making of the Badge") | Personal blog + Hackaday submission | Reach the #BadgeLife and maker communities. Time for pre-conference buzz. |
| **Mid Feb** | Post F ("The Badge Culture") | LinkedIn + Twitter/X | Cross-pollinate audiences. Security community meets AI community. |
| **Late Feb** | Blog B ("Building a Serial Test Protocol") | dev.to | Technical credibility piece. Reaches embedded/QA engineers. |
| **Early Mar** | Post G ("The Parallel Agents") | LinkedIn | AI workflow angle for the agentic AI community. |
| **Mid Mar** | Blog D ("4 Agents, 1 Architect") | LinkedIn Article + Medium | Thought leadership piece. Positions Carl for speaking invitations. |
| **Late Mar** | Post B ("The Supervisor Pattern") | LinkedIn | Reinforce the core narrative one month before the event. |
| **Ongoing** | Reply to comments, engage with shares, connect with people who resonate | LinkedIn | Community building. Every comment is a relationship. |

### Phase 3: Pre-Conference Hype (April 1-24, 2026)

| Timing | Content | Platform | Goal |
|--------|---------|----------|------|
| **April 1** | "One month out" post with badge photo/video teaser | LinkedIn + Twitter/X | Visual content. Show the physical badge. |
| **April 7** | Post about the bot API and AI competition angle | LinkedIn + BSides community channels | Drive interest in the game server competition. Get people pre-loading the repo. |
| **April 14** | "The firmware is ready" post with final feature list | LinkedIn | Confidence and closure on the build narrative. |
| **April 21** | "See you in KC" post — personal, short, excited | LinkedIn | Human moment before the event. |
| **April 24** | Quick post/story: "Tomorrow. Ad Astra." | LinkedIn + Twitter/X + Instagram | Anticipation. |

### Phase 4: Conference & Aftermath (April 25 - May 2026)

| Timing | Content | Platform | Goal |
|--------|---------|----------|------|
| **April 25 (day of)** | Live photos/videos of people using the badges | Twitter/X + LinkedIn + Instagram | Real-time social proof. Tag BSidesKC. |
| **April 26-27** | "What happened at BSidesKC" recap post with best moments | LinkedIn | Ride the post-conference engagement wave. |
| **Early May** | Reflective blog post: "What I Learned Building a Conference Badge with AI" | LinkedIn Article + Medium | Lessons learned piece. The most shareable long-form content will come from hindsight. |
| **Mid May** | Post about community contributions, forks, and what's next | LinkedIn | Transition from "I built this" to "we're building this." |
| **Ongoing** | Submit Talk A or Talk D to other conferences (BSides events, DevOpsDays, cloud summits) | Conference CFPs | Extend the story beyond BSidesKC. |

---

## 5. Audience-Specific Angles

### For IT Leaders / Architects

**Core Message:** You already have the skills to direct AI development. System thinking, decomposition, quality gates, and integration patterns — that's what AI supervision requires. The badge project is proof of concept.

**What They Care About:**
- Implications for team structure and hiring
- ROI of AI-assisted development
- Risk management (what happens when AI code crashes?)
- How this maps to enterprise-scale work

**Content to Lead With:** Post B ("The Supervisor Pattern"), Post E ("The Enterprise Architect's Secret"), Blog D ("4 Agents, 1 Architect")

**Where They Are:** LinkedIn (primary), Gartner conferences, Open Group events, enterprise architecture Slack communities

**Tone:** Strategic, measured, focused on organizational implications. Avoid hype. Emphasize the rigor (testing, issue tracking, quality gates).

---

### For Developers / Engineers

**Core Message:** AI-generated code still needs real engineering discipline — testing, debugging, architecture. The tools are changing; the principles aren't. Also: here's an open source project with interesting technical problems you can contribute to.

**What They Care About:**
- Technical depth (show the code, show the crashes, show the fixes)
- Whether this actually works or is just a demo
- The testing framework (BSTP, soak testing, MCP server)
- Open source contribution opportunities

**Content to Lead With:** Post C ("The Bug Hunt"), Blog B ("Building a Serial Test Protocol"), Talk C (testing talk)

**Where They Are:** dev.to, Hacker News, Reddit (r/embedded, r/esp32, r/programming), GitHub, Twitter/X

**Tone:** Technical, honest about failures, show-don't-tell. Developers respect the crash stories more than the feature list. Lead with what broke.

---

### For the Security Conference Community

**Core Message:** BSidesKC 2026 badges are going to be worth the trip. Full game, social features, hackable firmware, AI competition via bot API. Open source so you can start hacking before you arrive. Ad Astra.

**What They Care About:**
- Is the badge cool? (Yes — game, screensaver, BLE social, LEDs)
- Can I hack it? (Yes — open source, ESP32, documented)
- What's the social/competitive angle? (Bot API, AI competitions, BLE presence)
- Badge-to-badge interaction (crew roster, messaging, presence detection)

**Content to Lead With:** Post F ("The Badge Culture"), Blog C ("Ad Astra: The Making of the Badge"), Talk B (lightning talk)

**Where They Are:** BSides Slack/Discord channels, InfoSec Twitter/X, conference-specific forums, #BadgeLife communities, Hackaday

**Tone:** Community-first, enthusiastic, inclusive. Acknowledge BadgePirates' hardware design. Emphasize the open source and hackable nature. The AI angle is interesting but secondary — the badge itself is the star.

---

### For the AI/ML Community

**Core Message:** This is what agentic AI development looks like in practice — not a toy demo, but a complex embedded system with real hardware constraints, parallel agents, and a human architect in the loop. The workflow patterns matter more than the product.

**What They Care About:**
- Multi-agent coordination patterns
- Human-in-the-loop architecture decisions
- MCP server for hardware interaction
- Where AI agents failed and needed human intervention
- Reproducibility of the workflow

**Content to Lead With:** Post G ("The Parallel Agents"), Blog D ("4 Agents, 1 Architect"), Talk D ("The Architect's Unfair Advantage")

**Where They Are:** Twitter/X (AI community), LinkedIn, AI-focused Discords, Latent Space podcast community, AI engineering conferences

**Tone:** Practitioner-focused, workflow-oriented, honest about limitations. This community is saturated with hype — Carl's credibility comes from being a user, not a vendor. Emphasize what actually happened, not what's theoretically possible.

---

### For the Maker / Hardware Community

**Core Message:** An ESP32-S3 badge with a touchscreen, NeoPixels, and a buzzer running a full Lunar Lander game, 9-scene screensaver, and BLE social features. The firmware is open source and the testing framework is backportable to your projects.

**What They Care About:**
- Hardware specs and capabilities
- The game engine on a 320x240 display (how does it perform?)
- LED animation patterns
- Can they adapt this for their own badge/hardware project?
- The soak testing framework for their own embedded work

**Content to Lead With:** Blog C ("Ad Astra: The Making of the Badge"), Blog B ("Building a Serial Test Protocol"), Talk B (lightning talk)

**Where They Are:** Hackaday, Reddit (r/esp32, r/arduino, r/electronics), maker Discords, Adafruit/SparkFun communities, Hackaday Supercon

**Tone:** Build-focused, specs-forward, generous with technical details. This community respects people who ship real hardware. The AI angle is a curiosity, not the hook — lead with the badge, follow with how it was built.

---

## Quick Reference: Top 3 Moves to Make First

1. **Post "The Confession" (Post A) on LinkedIn this week.** It's the strongest hook, establishes the narrative, and everything else builds on it. See what resonates.

2. **Write Blog A ("I'm Not a Developer") within two weeks.** This is the canonical long-form piece. Every future post and talk can reference back to it.

3. **Submit Talk A ("I Can't Code") to BSidesKC's CFP as soon as it opens.** Home game, home crowd, live badge demo. This is the talk that launches speaking opportunities at other events.

---

*Ad Astra* 🚀
