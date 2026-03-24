# AI Development Instructions

## Agent Workflow

This project uses a supervisor/subagent pattern for all AI-assisted development.

### Roles

**Supervisor (you):**
- Plan and coordinate work
- Break tasks into discrete units
- Delegate ALL implementation to subagents via `coding-agent`
- Review subagent output
- Track progress against GitHub Issues
- Make architectural decisions
- Never write code directly — always delegate

**Subagent (`coding-agent`):**
- Reads and writes files
- Implements code changes
- Runs commands (build, test, git)
- Reports results back to supervisor

### Rules

1. **Always delegate code work.** File creation, code edits, running builds/tests, git operations — send to `coding-agent`.
2. **Batch independent work.** If multiple tasks have no dependencies, send them to parallel subagents.
3. **Keep context small.** Each subagent call should be a focused task with clear inputs and expected outputs. Don't dump the entire project history.
4. **Provide relevant context.** Tell the subagent what files to read, what patterns to follow, and what the acceptance criteria are.
5. **Verify before moving on.** After a subagent completes work, confirm the output meets the issue's acceptance criteria before closing.

### Task Flow

```
1. Pick next issue from GitHub (check dependencies are met)
2. Delegate implementation to coding-agent with:
   - Files to read/modify
   - Patterns to follow (reference existing code)
   - Acceptance criteria from the issue
3. Review subagent output
4. If needed, delegate fixes
5. Delegate: commit, push, close issue
6. Repeat
```

### Example Delegation

```
"Read include/Game/LunarConfig.h and src/Game/LunarPhysics.cpp, then create
src/Game/LunarTerrain.cpp implementing the Terrain struct and functions
described in issue #3. Follow the same header/include pattern as LunarPhysics.
The terrain_generate() function should use random walk with step/variation/zone
settings per difficulty level. Run `pio test -e native` after to verify tests pass."
```

## Project References

- **Porting plan:** `PORTING_PLAN.md`
- **Project structure:** `PROJECT_STRUCTURE.md`
- **Task tracking:** [GitHub Issues](https://github.com/carlfugate/lunarlander-badge/issues)
- **UI mockups:** `mockups/badge-mockups.html`
- **Feature flags:** `include/FeatureFlags.h`
- **Game config:** `include/Game/LunarConfig.h`
- **Base firmware patterns:** `src/QA/Menu.cpp` (menu integration), `include/lv_conf.h` (LVGL config)
- **Original game logic:** `~/Documents/github/lunarlander/server/game/` (physics.py, terrain.py, session.py)
- **Badge hardware:** `include/pins.h`, `include/Hardware/`
